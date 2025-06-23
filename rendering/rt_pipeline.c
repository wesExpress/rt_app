#include "rt_pipeline.h"
#include "../app.h"
#include "../data.h"
#include "../entities.h"

#define CUBE_BLAS 0
#define TRI_BLAS  1
#define QUAD_BLAS 2

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
        desc.write      = true;

        if(!dm_renderer_create_texture(desc, &app_data->rt_data.image, context)) return false;

        desc.format = DM_TEXTURE_FORMAT_FLOAT_4;
        if(!dm_renderer_create_texture(desc, &app_data->rt_data.ray_image, context)) return false;
    }

    // === rt pipeline === 
    {
        dm_raytracing_pipeline_hit_group hit_group = {
            .type = DM_RT_PIPE_HIT_GROUP_TYPE_TRIANGLES, .name = "hit_group",
            .shaders[DM_RT_PIPE_HIT_GROUP_STAGE_CLOSEST] = "closest_hit",
            .flags = DM_RT_PIPE_HIT_GROUP_FLAG_CLOSEST,
        };

        dm_raytracing_pipeline_desc rt_pipe_desc = {  
            .shader_path = "assets/rt_shader.cso",
            .raygen = "ray_generation", .miss = "miss",
            .hit_groups[0]=hit_group, .hit_group_count=1,
            .payload_size=sizeof(ray_payload), .max_depth=1, .max_instance_count=MAX_ENTITIES
        };

        if(!dm_renderer_create_raytracing_pipeline(rt_pipe_desc, &app_data->rt_data.pipeline, context)) return false;
    }

    // === bottom-levels ===
    {
        dm_blas_desc blas_cube = { 
            .flags=DM_BLAS_GEOMETRY_FLAG_OPAQUE, .geometry_type=DM_BLAS_GEOMETRY_TYPE_TRIANGLES, .vertex_type=DM_BLAS_VERTEX_TYPE_FLOAT_3,
            .vertex_count=_countof(cube), .vertex_buffer=app_data->raster_data.vb_cube, .vertex_stride=sizeof(vertex), 
            .index_count=_countof(cube_indices), .index_buffer=app_data->raster_data.ib_cube, .index_type=INDEX_TYPE
        };
        if(!dm_renderer_create_blas(blas_cube, &app_data->rt_data.cube_blas, context)) return false;
        
        dm_blas_desc blas_tri = { 
            .flags=DM_BLAS_GEOMETRY_FLAG_OPAQUE, .geometry_type=DM_BLAS_GEOMETRY_TYPE_TRIANGLES, .vertex_type=DM_BLAS_VERTEX_TYPE_FLOAT_3,
            .vertex_count=_countof(triangle), .vertex_buffer=app_data->raster_data.vb_tri, .vertex_stride=sizeof(vertex), 
            .index_count=_countof(triangle_indices), .index_buffer=app_data->raster_data.ib_tri, .index_type=INDEX_TYPE
        };
        if(!dm_renderer_create_blas(blas_tri, &app_data->rt_data.triangle_blas, context)) return false;

        dm_blas_desc blas_quad = { 
            .flags=DM_BLAS_GEOMETRY_FLAG_OPAQUE, .geometry_type=DM_BLAS_GEOMETRY_TYPE_TRIANGLES, .vertex_type=DM_BLAS_VERTEX_TYPE_FLOAT_3,
            .vertex_count=_countof(quad), .vertex_buffer=app_data->raster_data.vb_quad, .vertex_stride=sizeof(vertex), 
            .index_count=_countof(quad_indices), .index_buffer=app_data->raster_data.ib_quad, .index_type=INDEX_TYPE
        };
        if(!dm_renderer_create_blas(blas_quad, &app_data->rt_data.quad_blas, context)) return false;
    }

    // === blas buffer ===
    {
        if(!dm_renderer_get_blas_gpu_address(app_data->rt_data.cube_blas,     &app_data->rt_data.blas_addresses[CUBE_BLAS], context)) return false;
        if(!dm_renderer_get_blas_gpu_address(app_data->rt_data.triangle_blas, &app_data->rt_data.blas_addresses[TRI_BLAS],  context)) return false;
        if(!dm_renderer_get_blas_gpu_address(app_data->rt_data.quad_blas,     &app_data->rt_data.blas_addresses[QUAD_BLAS], context)) return false;

        dm_storage_buffer_desc desc = { 0 };
        desc.write  = false;
        desc.size   = sizeof(size_t) * 10;
        desc.stride = sizeof(size_t);
        desc.data   = app_data->rt_data.blas_addresses;

        if(!dm_renderer_create_storage_buffer(desc, &app_data->rt_data.blas_buffer, context)) return false;
    }

    // fill in and create rt instance buffer
    {
        for(uint32_t i=0; i<MAX_ENTITIES; i++)
        {
            app_data->entities.rt_instances[i].id = i;
            app_data->entities.rt_instances[i].sbt_offset = 0;
            app_data->entities.rt_instances[i].mask = 0xFF;
            app_data->entities.rt_instances[i].blas_address = app_data->rt_data.blas_addresses[CUBE_BLAS];
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

        if(!dm_renderer_create_constant_buffer(desc, &app_data->rt_data.cb, context)) return false;

        desc.size = sizeof(rt_camera_data);
        desc.data = NULL;

        if(!dm_renderer_create_constant_buffer(desc, &app_data->rt_data.compute_cb, context)) return false;

        desc.size = sizeof(rt_image_data);

        if(!dm_renderer_create_constant_buffer(desc, &app_data->rt_data.compute_image_cb, context)) return false;
    }

    // === compute pipeline ===
    {
        dm_compute_pipeline_desc desc = {  
            .shader="assets/ray_directions.cso"
        };

        if(!dm_compute_create_compute_pipeline(desc, &app_data->rt_data.compute_pipeline, context)) return false;
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
    dm_memcpy(app_data->rt_data.scene_data.inv_view, app_data->camera.inv_view, sizeof(dm_mat4));
    dm_memcpy(app_data->rt_data.scene_data.inv_proj, app_data->camera.inv_proj, sizeof(dm_mat4));
    dm_memcpy(app_data->rt_data.scene_data.inv_vp,   app_data->camera.inv_view_proj, sizeof(dm_mat4));
#endif

    dm_memcpy(app_data->rt_data.scene_data.origin, app_data->camera.pos, sizeof(dm_vec3));
    dm_memcpy(app_data->rt_data.camera_data.position, app_data->camera.pos, sizeof(dm_vec4));

    app_data->rt_data.image_data.width  = context->renderer.width;
    app_data->rt_data.image_data.height = context->renderer.height;

    app_data->rt_data.scene_data.sky_color[0] = 0.2f;
    app_data->rt_data.scene_data.sky_color[1] = 0.5f;
    app_data->rt_data.scene_data.sky_color[2] = 0.7f;

    return true;
}

bool rt_pipeline_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    // compute stuff
    app_data->rt_data.compute_resources.camera_buffer = app_data->rt_data.compute_cb.descriptor_index;
    app_data->rt_data.compute_resources.image_data    = app_data->rt_data.compute_image_cb.descriptor_index;
    app_data->rt_data.compute_resources.ray_image     = app_data->rt_data.ray_image.descriptor_index;

    dm_compute_command_update_constant_buffer(&app_data->rt_data.camera_data, sizeof(rt_camera_data), app_data->rt_data.compute_cb, context);
    dm_compute_command_update_constant_buffer(&app_data->rt_data.image_data, sizeof(rt_image_data), app_data->rt_data.compute_image_cb, context);

    dm_compute_command_bind_compute_pipeline(app_data->rt_data.compute_pipeline, context);
    dm_compute_command_set_root_constants(0,3,0, &app_data->rt_data.compute_resources, context);
    dm_compute_command_dispatch(context->renderer.width, context->renderer.height, 1, context);

    // resource indices
    app_data->rt_data.resources.acceleration_structure = app_data->rt_data.tlas.descriptor_index;
    app_data->rt_data.resources.image                  = app_data->rt_data.image.descriptor_index;
    app_data->rt_data.resources.constant_buffer        = app_data->rt_data.cb.descriptor_index;
    app_data->rt_data.resources.material_buffer        = app_data->entities.material_sb.descriptor_index;
    app_data->rt_data.resources.ray_image              = app_data->rt_data.ray_image.descriptor_index;

    // update render objects
    dm_render_command_update_constant_buffer(&app_data->rt_data.scene_data, sizeof(scene_cb), app_data->rt_data.cb, context);
    dm_render_command_update_tlas(MAX_ENTITIES, app_data->rt_data.tlas, context);

    // render
    dm_render_command_bind_raytracing_pipeline(app_data->rt_data.pipeline, context);
    dm_render_command_set_root_constants(0,5,0, &app_data->rt_data.resources, context);
    dm_render_command_dispatch_rays(context->renderer.width, context->renderer.height, app_data->rt_data.pipeline, context);
    dm_render_command_copy_image_to_screen(app_data->rt_data.image, context);

    return true;
}
