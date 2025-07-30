#include "rt_pipeline.h"
#include "../app.h"

typedef struct ray_payload_t
{
    dm_vec4  color;
    uint32_t bounce_count;
} ray_payload;

bool rt_pipeline_init(dm_scene scene, dm_context* context)
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

        uint32_t* data = dm_alloc(sizeof(uint32_t) * context->renderer.width * context->renderer.height);
        for(uint32_t x=0; x<context->renderer.width; x++)
        {
            for(uint32_t y=0; y<context->renderer.height; y++)
            {
                data[x + y * context->renderer.width] = dm_random_uint32(context);
            }
        }
        
        desc.data = data;

        if(!dm_renderer_create_texture(desc, &app_data->rt_data.random_image, context)) return false;

        dm_free((void**)&data);
    }

    // === rt pipeline === 
    {
        dm_raytracing_pipeline_hit_group hit_group = {
            .type = DM_RT_PIPE_HIT_GROUP_TYPE_TRIANGLES, .name = "hit_group",
#ifdef DM_DIRECTX12
            .shaders[DM_RT_PIPE_HIT_GROUP_STAGE_CLOSEST] = "closest_hit",
#elif defined(DM_VULKAN)
            .shaders[DM_RT_PIPE_HIT_GROUP_STAGE_CLOSEST] = "assets/shaders/rt_chit.spv",
#endif
            .flags = DM_RT_PIPE_HIT_GROUP_FLAG_CLOSEST,
        };

        dm_raytracing_pipeline_desc rt_pipe_desc = {  
#ifdef DM_DIRECTX12
            .shader_path = "assets/shaders/rt_shader.cso",
            .raygen = "ray_generation", 
            .misses = { { "miss" }, { "shadow_miss" } }, 
#elif defined(DM_VULKAN)
            .raygen="assets/shaders/rt_gen.spv", .misses={ { "assets/shaders/rt_miss.spv" }, { "assets/shaders/rt_shadow_miss.spv" } },
#endif
            .miss_count=2, .hit_groups={ hit_group }, .hit_group_count=1,
            .payload_size=sizeof(ray_payload), .max_depth=3, 
        };

        if(!dm_renderer_create_raytracing_pipeline(rt_pipe_desc, &app_data->rt_data.pipeline, context)) return false;
    }

    // === bottom-levels ===
    app_data->rt_data.blas = dm_alloc(sizeof(dm_resource_handle) * scene.mesh_count);
    app_data->rt_data.blas_addresses = dm_alloc(sizeof(size_t) * scene.mesh_count);
    {
        for(uint32_t i=0; i<scene.mesh_count; i++)
        {
            dm_blas_desc desc = { 0 };
            desc.geometry_type = DM_BLAS_GEOMETRY_TYPE_TRIANGLES;
            desc.flags         = DM_BLAS_GEOMETRY_FLAG_OPAQUE;
            desc.vertex_type   = DM_BLAS_VERTEX_TYPE_FLOAT_3;
            desc.vertex_buffer = scene.meshes[i].vb;
            desc.index_buffer  = scene.meshes[i].ib;
            desc.vertex_count  = scene.meshes[i].vertex_count;
            desc.vertex_stride = scene.meshes[i].vertex_stride;
            desc.index_type    = scene.meshes[i].index_type;
            desc.index_count   = scene.meshes[i].index_count;

            if(!dm_renderer_create_blas(desc, &app_data->rt_data.blas[i], context)) return false;
            if(!dm_renderer_get_blas_gpu_address(app_data->rt_data.blas[i], &app_data->rt_data.blas_addresses[i], context)) return false;
        }
    }

    // === fill in and create rt instance buffer ===
    {
        app_data->rt_data.instances = dm_alloc(sizeof(dm_raytracing_instance) * scene.node_count);

        for(uint32_t i=0; i<scene.node_count; i++)
        {
            dm_raytracing_instance* instance = &app_data->rt_data.instances[i];
            dm_scene_node node = scene.nodes[i];

            instance->blas_address = app_data->rt_data.blas_addresses[node.mesh_index];
            instance->mask         = 0xFF;
            instance->id           = i;

            dm_mat4 dum;
            dm_mat4_transpose(node.model_matrix, dum);
            dm_memcpy(instance->transform, dum, sizeof(float) * 4 * 3);
        }

        dm_storage_buffer_desc desc = { 0 };
        desc.size   = scene.node_count * sizeof(dm_raytracing_instance);
        desc.stride = sizeof(dm_raytracing_instance);
        desc.data   = app_data->rt_data.instances;

        if(!dm_renderer_create_storage_buffer(desc, &app_data->rt_data.instance_sb, context)) return false;
    }

    // === top-level ===
    {
        dm_tlas_desc tlas_desc = { 
            .instance_count=scene.node_count, .instance_buffer=app_data->rt_data.instance_sb
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

    //
    app_data->rt_data.image_width  = context->renderer.width;
    app_data->rt_data.image_height = context->renderer.height;

    return true;
}

void rt_pipeline_shutdown(dm_context* context)
{
    application_data* app_data = context->app_data;

    dm_free((void**)&app_data->rt_data.instances);
    dm_free((void**)&app_data->rt_data.blas);
    dm_free((void**)&app_data->rt_data.blas_addresses);
}

bool rt_pipeline_update(dm_scene scene, dm_context* context)
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

    for(uint8_t i=0; i<app_data->sponza_scene.mesh_count; i++)
    {
        if(!dm_renderer_get_blas_gpu_address(app_data->rt_data.blas[i], &app_data->rt_data.blas_addresses[i], context)) return false;
    }

    // render udpates
    dm_render_command_update_constant_buffer(&app_data->rt_data.camera_data, sizeof(rt_camera_data), app_data->rt_data.camera_cb, context);
    dm_render_command_update_constant_buffer(&app_data->rt_data.scene_data, sizeof(scene_cb), app_data->rt_data.scene_cb, context);

    dm_render_command_update_tlas(scene.node_count, app_data->rt_data.tlas, context);

    if(app_data->rt_data.image_width != context->renderer.width || app_data->rt_data.image_height != context->renderer.height)
    {
        app_data->rt_data.image_width  = context->renderer.width;
        app_data->rt_data.image_height = context->renderer.height;

        dm_render_command_resize_texture(context->renderer.width, context->renderer.height, app_data->rt_data.image, context);
        dm_render_command_resize_texture(context->renderer.width, context->renderer.height, app_data->rt_data.random_image, context);
    }

    return true;
}

void rt_pipeline_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    // resource indices
    app_data->rt_data.resources.acceleration_structure = app_data->rt_data.tlas.descriptor_index;
    app_data->rt_data.resources.image                  = app_data->rt_data.image.descriptor_index;
    app_data->rt_data.resources.random_image           = app_data->rt_data.random_image.descriptor_index;
    app_data->rt_data.resources.camera_data_index      = app_data->rt_data.camera_cb.descriptor_index;
    app_data->rt_data.resources.scene_data_index       = app_data->rt_data.scene_cb.descriptor_index;
    app_data->rt_data.resources.node_buffer_index      = app_data->node_buffer.descriptor_index;
    app_data->rt_data.resources.material_buffer_index  = app_data->material_sb.descriptor_index;
    app_data->rt_data.resources.mesh_buffer_index      = app_data->mesh_sb.descriptor_index;
    app_data->rt_data.resources.light_buffer_index     = app_data->light_buffer.descriptor_index;

    // render
    dm_render_command_bind_raytracing_pipeline(app_data->rt_data.pipeline, context);
    dm_render_command_set_root_constants(0,9,0, &app_data->rt_data.resources, context);
    dm_render_command_dispatch_rays(context->renderer.width, context->renderer.height, app_data->rt_data.pipeline, context);
}

