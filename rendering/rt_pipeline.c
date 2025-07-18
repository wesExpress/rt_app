#include "rt_pipeline.h"
#include "../app.h"
#include "../entities.h"

#include <stdio.h>

typedef struct ray_payload_t
{
    dm_vec4 color;
} ray_payload;

bool rt_pipeline_init(dm_context* context)
{
    application_data* app_data = context->app_data;

    // === image(s) ===
    {
        dm_texture_desc desc = { 0 };
        desc.format     = DM_TEXTURE_FORMAT_BYTE_4_UNORM;
        desc.width      = context->renderer.width;
        desc.height     = context->renderer.height;
        desc.n_channels = 4;

        if(!dm_renderer_create_texture(desc, &app_data->rt_data.image, context)) return false;
    }

    // === rt pipeline === 
    {
        dm_raytracing_pipeline_hit_group hit_group = {
            .type = DM_RT_PIPE_HIT_GROUP_TYPE_TRIANGLES, .name = "hit_group",
#ifdef DM_DIRECTX12
            .shaders[DM_RT_PIPE_HIT_GROUP_STAGE_CLOSEST] = "closest_hit",
#elif defined(DM_VULKAN)
            .shaders[DM_RT_PIPE_HIT_GROUP_STAGE_CLOSEST] = "assets/shaders/rchit.spv",
#endif
            .flags = DM_RT_PIPE_HIT_GROUP_FLAG_CLOSEST,
        };

        dm_raytracing_pipeline_desc rt_pipe_desc = {  
#ifdef DM_DIRECTX12
            .shader_path = "assets/shaders/rt_shader.cso",
            .raygen = "ray_generation", .miss = "miss",
#elif defined(DM_VULKAN)
            .raygen="assets/shaders/rgen.spv", .miss="assets/shaders/rmiss.spv",
#endif
            .hit_groups[0]=hit_group, .hit_group_count=1,
            .payload_size=sizeof(ray_payload), .max_depth=1, .max_instance_count=MAX_ENTITIES
        };

        if(!dm_renderer_create_raytracing_pipeline(rt_pipe_desc, &app_data->rt_data.pipeline, context)) return false;
    }

    // === bottom-levels ===
    {
        for(uint32_t i=0; i<app_data->mesh_count; i++)
        {
            dm_blas_desc desc = { 0 };
            desc.geometry_type = DM_BLAS_GEOMETRY_TYPE_TRIANGLES;
            desc.flags         = DM_BLAS_GEOMETRY_FLAG_OPAQUE;
            desc.vertex_type   = DM_BLAS_VERTEX_TYPE_FLOAT_3;
            desc.vertex_count  = app_data->meshes[i].vertex_count;
            desc.vertex_buffer = app_data->meshes[i].vb;
            desc.vertex_stride = app_data->meshes[i].vertex_stride;

            if(!dm_renderer_create_blas(desc, &app_data->rt_data.blas[i], context)) return false;
        }
    }

    // === blas buffer ===
    {
        for(uint16_t i=0; i<app_data->mesh_count; i++)
        {
            if(!dm_renderer_get_blas_gpu_address(app_data->rt_data.blas[i], &app_data->rt_data.blas_addresses[i], context)) return false;
        }

        dm_storage_buffer_desc desc = { 0 };
        desc.write  = false;
        desc.size   = sizeof(size_t) * 10;
        desc.stride = sizeof(size_t);
        desc.data   = app_data->rt_data.blas_addresses;

        if(!dm_renderer_create_storage_buffer(desc, &app_data->rt_data.blas_buffer, context)) return false;
    }

    // === fill in and create rt instance buffer ===
    {
        for(uint32_t i=0; i<MAX_ENTITIES; i++)
        {
            app_data->entities.rt_instances[i].id = i;
            app_data->entities.rt_instances[i].sbt_offset = 0;
            app_data->entities.rt_instances[i].mask = 0xFF;
            app_data->entities.rt_instances[i].blas_address = app_data->rt_data.blas_addresses[0];

            app_data->entities.rt_instances[i].transform[0][0] = 1;
            app_data->entities.rt_instances[i].transform[1][1] = 1;
            app_data->entities.rt_instances[i].transform[2][2] = 1;

            app_data->entities.rt_instances[i].transform[0][3] = dm_random_float(context) * 50.f - 25.f;
            app_data->entities.rt_instances[i].transform[1][3] = dm_random_float(context) * 50.f - 25.f;
            app_data->entities.rt_instances[i].transform[2][3] = dm_random_float(context) * 50.f - 25.f;
        }
        
        dm_storage_buffer_desc desc = { 0 };
        desc.write  = true;
        desc.size   = MAX_ENTITIES * sizeof(dm_raytracing_instance);
        desc.stride = sizeof(dm_raytracing_instance);
        desc.data   = app_data->entities.rt_instances;

        if(!dm_renderer_create_storage_buffer(desc, &app_data->entities.rt_instance_sb, context)) return false;
    }

    // === top-level ===
    {
        dm_tlas_desc tlas_desc = { 
            .instance_count=MAX_ENTITIES, .instance_buffer=app_data->entities.rt_instance_sb
        };

        if(!dm_renderer_create_tlas(tlas_desc, &app_data->rt_data.tlas, context)) return false; 
    }

    // === constant buffer(s) ===
    {
        dm_constant_buffer_desc desc = { 
            .size=sizeof(scene_cb), .data=&app_data->rt_data.scene_data
        };

        if(!dm_renderer_create_constant_buffer(desc, &app_data->rt_data.scene_cb, context)) return false;

        desc.size = sizeof(rt_camera_data);
        desc.data = &app_data->rt_data.camera_data;

        if(!dm_renderer_create_constant_buffer(desc, &app_data->rt_data.camera_cb, context)) return false;
    }

    return true;
}

bool rt_pipeline_update(dm_context* context)
{
    application_data* app_data = context->app_data;

    // constant buffer data
#ifdef DM_DIRECTX12
    dm_mat4_transpose(app_data->camera.inv_view, app_data->rt_data.camera_data.inv_view);
    dm_mat4_transpose(app_data->camera.inv_proj, app_data->rt_data.camera_data.inv_proj);
#elif defined(DM_VULKAN)
    dm_memcpy(app_data->rt_data.camera_data.inv_view, app_data->camera.inv_view, sizeof(dm_mat4));
    dm_memcpy(app_data->rt_data.camera_data.inv_proj, app_data->camera.inv_proj, sizeof(dm_mat4));
#endif

    dm_memcpy(app_data->rt_data.camera_data.position, app_data->camera.pos, sizeof(dm_vec4));

    app_data->rt_data.scene_data.sky_color[0] = 0.02f;
    app_data->rt_data.scene_data.sky_color[1] = 0.02f;
    app_data->rt_data.scene_data.sky_color[2] = 0.02f;

    // blas addresses
    for(uint16_t i=0; i<app_data->mesh_count; i++)
    {
        if(!dm_renderer_get_blas_gpu_address(app_data->rt_data.blas[i], &app_data->rt_data.blas_addresses[i], context)) return false;
    }

    // render udpates
    dm_render_command_update_storage_buffer(app_data->rt_data.blas_addresses, sizeof(app_data->rt_data.blas_addresses), app_data->rt_data.blas_buffer, context);
    dm_render_command_update_constant_buffer(&app_data->rt_data.camera_data, sizeof(rt_camera_data), app_data->rt_data.camera_cb, context);
    dm_render_command_update_constant_buffer(&app_data->rt_data.scene_data, sizeof(scene_cb), app_data->rt_data.scene_cb, context);

    dm_render_command_update_tlas(MAX_ENTITIES, app_data->rt_data.tlas, context);

    return true;
}

bool rt_pipeline_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    // resource indices
    app_data->rt_data.resources.acceleration_structure = app_data->rt_data.tlas.descriptor_index;
    app_data->rt_data.resources.image                  = app_data->rt_data.image.descriptor_index;
    app_data->rt_data.resources.camera_data_index      = app_data->rt_data.camera_cb.descriptor_index;
    app_data->rt_data.resources.scene_data_index       = app_data->rt_data.scene_cb.descriptor_index;
    app_data->rt_data.resources.material_buffer        = app_data->entities.material_sb.descriptor_index;

    if(app_data->rt_data.resized) 
    {
        dm_render_command_resize_texture(context->renderer.width, context->renderer.height, app_data->rt_data.image, context);
        app_data->rt_data.resized = false;
    }

    // render
    dm_render_command_bind_raytracing_pipeline(app_data->rt_data.pipeline, context);
    dm_render_command_set_root_constants(0,5,0, &app_data->rt_data.resources, context);
    dm_render_command_dispatch_rays(context->renderer.width, context->renderer.height, app_data->rt_data.pipeline, context);

    return true;
}

