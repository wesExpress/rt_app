#include "rt_pipeline.h"
#include "../app.h"
#include "../data.h"

typedef struct ray_payload_t
{
    dm_vec4 color;
} ray_payload;

#define CUBE_BLAS 0
#define TRI_BLAS  1
#define QUAD_BLAS 2

bool rt_pipeline_init(dm_context* context)
{
    application_data* app_data = context->app_data;

    // === image ===
    {
        dm_texture_desc desc = { 0 };
        desc.format     = DM_TEXTURE_FORMAT_BYTE_4_UNORM;
        desc.width      = context->renderer.width;
        desc.height     = context->renderer.height;
        desc.n_channels = 4;
        desc.write      = true;

        if(!dm_renderer_create_texture(desc, &app_data->rt_data.image, context)) return false;
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
            .payload_size=sizeof(ray_payload), .max_depth=1, .max_instance_count=MAX_INSTANCES
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

        if(!dm_renderer_get_blas_gpu_address(app_data->rt_data.cube_blas,     &app_data->rt_data.blas_addresses[CUBE_BLAS], context)) return false;
        if(!dm_renderer_get_blas_gpu_address(app_data->rt_data.triangle_blas, &app_data->rt_data.blas_addresses[TRI_BLAS],  context)) return false;
        if(!dm_renderer_get_blas_gpu_address(app_data->rt_data.quad_blas,     &app_data->rt_data.blas_addresses[QUAD_BLAS], context)) return false;
    }

    // instances with correct blas
    for(uint32_t i=0; i<MAX_INSTANCES; i++)
    {
        // misc
        app_data->raytracing_instances[i].blas_address = app_data->rt_data.blas_addresses[CUBE_BLAS];

        app_data->raytracing_instances[i].id = i;
        app_data->raytracing_instances[i].sbt_offset = 0;
        app_data->raytracing_instances[i].mask = 0xFF;

        // calculate transform
        dm_mat4 model;
        dm_mat4 rotation;

        dm_mat4_identity(model);

        transform t = app_data->transforms[i];

        dm_mat4_rotate_from_quat(t.orientation, rotation);

        dm_mat_scale(model, t.scale, model);
        dm_mat4_mul_mat4(model, rotation, model);
        dm_mat_translate(model, t.position, model);
        dm_mat4_transpose(model, model);

        dm_memcpy(app_data->raytracing_instances[i].transform, model, sizeof(float) * 3 * 4);

        dm_quat delta_rot, new_rot;
        dm_vec3 rotate = { 0,0.001f,0 };
        dm_vec3_mul_quat(rotate, t.orientation, delta_rot);
        dm_quat_add_quat(t.orientation, delta_rot, new_rot);
        dm_quat_norm(new_rot, t.orientation);
        app_data->transforms[i] = t;
    }

    // === top-level ===
    {
        dm_tlas_desc tlas_desc = { 
            .instance_count=MAX_INSTANCES, .instances=app_data->raytracing_instances
        };

        if(!dm_renderer_create_tlas(tlas_desc, &app_data->rt_data.tlas, context)) return false; 
    }

    // === constant buffer ===
    {
        dm_constant_buffer_desc desc = { 
            .size=sizeof(scene_cb), .data=&app_data->rt_data.scene_data
        };

        if(!dm_renderer_create_constant_buffer(desc, &app_data->rt_data.cb, context)) return false;
    }

    return true;
}

bool rt_pipeline_update(dm_context* context)
{
    application_data* app_data = context->app_data;

    // TODO: offput onto gpu or something? but how?
    for(uint32_t i=0; i<MAX_INSTANCES; i++)
    {
        // misc
        app_data->raytracing_instances[i].blas_address = app_data->rt_data.blas_addresses[TRI_BLAS];

        app_data->raytracing_instances[i].id = i;
        app_data->raytracing_instances[i].sbt_offset = 0;
        app_data->raytracing_instances[i].mask = 0xFF;

        // calculate transform
        dm_mat4 model;
        dm_mat4 rotation;

        dm_mat4_identity(model);

        transform t = app_data->transforms[i];

        dm_mat4_rotate_from_quat(t.orientation, rotation);

        dm_mat_scale(model, t.scale, model);
        dm_mat4_mul_mat4(model, rotation, model);
        dm_mat_translate(model, t.position, model);
        dm_mat4_transpose(model, model);

        dm_memcpy(app_data->raytracing_instances[i].transform, model, sizeof(float) * 3 * 4);

        dm_quat delta_rot, new_rot;
        dm_vec3 rotate = { 0,0.001f,0 };
        dm_vec3_mul_quat(rotate, t.orientation, delta_rot);
        dm_quat_add_quat(t.orientation, delta_rot, new_rot);
        dm_quat_norm(new_rot, t.orientation);
        app_data->transforms[i] = t;
    }

    return true;
}

bool rt_pipeline_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    app_data->rt_data.resources.acceleration_structure = app_data->rt_data.tlas.descriptor_index;
    app_data->rt_data.resources.image                  = app_data->rt_data.image.descriptor_index;
    app_data->rt_data.resources.constant_buffer        = app_data->rt_data.cb.descriptor_index;

#ifdef DM_DIRECTX12
    dm_mat4_transpose(app_data->camera.inv_view, app_data->rt_data.scene_data.inv_view);
    dm_mat4_transpose(app_data->camera.inv_proj, app_data->rt_data.scene_data.inv_proj);
#elif defined(DM_VULKAN)
    dm_memcpy(app_data->rt_data.scene_data.inv_view, app_data->camera.inv_view, sizeof(dm_mat4));
    dm_memcpy(app_data->rt_data.scene_data.inv_proj, app_data->camera.inv_proj, sizeof(dm_mat4));
#endif
    dm_memcpy(app_data->rt_data.scene_data.origin, app_data->camera.pos, sizeof(dm_vec3));
    
    dm_render_command_update_constant_buffer(&app_data->rt_data.scene_data, sizeof(scene_cb), app_data->rt_data.cb, context);
    dm_render_command_update_tlas(app_data->raytracing_instances, sizeof(app_data->raytracing_instances), MAX_INSTANCES, app_data->rt_data.tlas, context);

    dm_render_command_bind_raytracing_pipeline(app_data->rt_data.pipeline, context);
    dm_render_command_set_root_constants(0,3,0, &app_data->rt_data.resources, context);
    dm_render_command_dispatch_rays(context->renderer.width, context->renderer.height, app_data->rt_data.pipeline, context);
    dm_render_command_copy_image_to_screen(app_data->rt_data.image, context);

    return true;
}

