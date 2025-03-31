#include "rt_pipeline.h"
#include "../app.h"
#include "../data.h"

#define CUBE_BLAS 0

bool rt_pipeline_init(dm_resource_handle vb, dm_resource_handle ib, dm_context* context)
{
    application_data* app_data = context->app_data;

    // === acceleration structure ===
    {
        dm_acceleration_structure_desc desc = { 0 };
        desc.tlas.blas[0].flags         = DM_BLAS_GEOMETRY_FLAG_OPAQUE;
        desc.tlas.blas[0].geometry_type = DM_BLAS_GEOMETRY_TYPE_TRIANGLES;
        desc.tlas.blas[0].vertex_type   = DM_BLAS_VERTEX_TYPE_FLOAT_3;
        desc.tlas.blas[0].vertex_count  = sizeof(cube) / sizeof(vertex); 
        desc.tlas.blas[0].vertex_buffer = vb;
        desc.tlas.blas[0].vertex_stride = sizeof(vertex);
        desc.tlas.blas[0].index_type    = INDEX_TYPE;
        desc.tlas.blas[0].index_count   = _countof(cube_indices);
        desc.tlas.blas[0].index_buffer  = ib;

        desc.tlas.blas_count     = 1;
        desc.tlas.instance_count = MAX_INSTANCES;
        desc.tlas.instances = app_data->raytracing_instances;
        
        if(!dm_renderer_create_acceleration_structure(desc, &app_data->rt_data.acceleration_structure, context)) return false;
    }

    // === rt pipeline === 
    {
        dm_raytracing_pipeline_desc rt_pipe_desc = { 0 };

        rt_pipe_desc.hit_group_count = 1;
        rt_pipe_desc.hit_groups[0].type = DM_RT_PIPE_HIT_GROUP_TYPE_TRIANGLES;
        dm_strcpy(rt_pipe_desc.hit_groups[0].name, "hit_group");
        dm_strcpy(rt_pipe_desc.hit_groups[0].shaders[DM_RT_PIPE_HIT_GROUP_STAGE_CLOSEST], "closest_hit");
        dm_strcpy(rt_pipe_desc.hit_groups[0].path, "assets/rt_shader.cso");
        rt_pipe_desc.hit_groups[0].flags |= DM_RT_PIPE_HIT_GROUP_FLAG_CLOSEST;

        rt_pipe_desc.global_descriptor_groups[0].descriptors[0] = DM_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;
        rt_pipe_desc.global_descriptor_groups[0].descriptors[1] = DM_DESCRIPTOR_TYPE_WRITE_TEXTURE;
        rt_pipe_desc.global_descriptor_groups[0].descriptors[2] = DM_DESCRIPTOR_TYPE_CONSTANT_BUFFER;
        rt_pipe_desc.global_descriptor_groups[0].count          = 3;
        rt_pipe_desc.global_descriptor_group_count              = 1;

        //if(!dm_renderer_create_raytracing_pipeline(rt_pipe_desc, &app_data->rt_pipe, context)) return false;
    }

    return true;
}

bool rt_pipeline_update(dm_context* context)
{
    application_data* app_data = context->app_data;

    if(!dm_renderer_get_blas_gpu_address(app_data->rt_data.acceleration_structure, CUBE_BLAS, &app_data->rt_data.blas_addresses[CUBE_BLAS], context)) return false;

    return true;
}

bool rt_pipeline_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    dm_render_command_update_acceleration_structure(app_data->raytracing_instances, sizeof(app_data->raytracing_instances), MAX_INSTANCES, app_data->rt_data.acceleration_structure, context);

    return true;
}

