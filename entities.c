#include "entities.h"
#include "app.h"

#include "rendering/debug_pipeline.h"

#ifndef DM_DEBUG
DM_INLINE
#endif
transform init_transform(float world_scale, dm_context* context)
{
    transform t;

    t.position[0] = dm_random_float(context) * world_scale - world_scale * 0.5f;
    t.position[1] = dm_random_float(context) * world_scale - world_scale * 0.5f;
    t.position[2] = dm_random_float(context) * world_scale - world_scale * 0.5f;

    t.scale[0] = dm_random_float(context) * 2.5f;
    t.scale[1] = dm_random_float(context) * 2.5f;
    t.scale[2] = dm_random_float(context) * 2.5f;

    t.orientation[0] = dm_random_float(context) * 2.f - 1.f;
    t.orientation[1] = dm_random_float(context) * 2.f - 1.f;
    t.orientation[2] = dm_random_float(context) * 2.f - 1.f;
    t.orientation[3] = 1.f;
    dm_quat_norm(t.orientation, t.orientation);

    return t;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
physics init_physics(dm_context* context)
{
    physics p;

    p.w[0] = dm_random_float(context) * 2.f - 1.f; 
    p.w[1] = dm_random_float(context) * 2.f - 1.f;
    p.w[2] = dm_random_float(context) * 2.f - 1.f;

    p.w[0] *= 0.005f;
    p.w[1] *= 0.005f;
    p.w[2] *= 0.005f;

    return p;
}

bool init_entities(dm_context* context)
{
    application_data* app_data = context->app_data;

    for(uint32_t i=0; i<MAX_ENTITIES; i++)
    {
        app_data->entities.transforms[i] = init_transform(50.f, context);
        app_data->entities.phys[i]       = init_physics(context);

        app_data->entities.materials[i].vb_index = app_data->raster_data.vb_cube.descriptor_index;
        app_data->entities.materials[i].ib_index = app_data->raster_data.ib_cube.descriptor_index;
    }

    // === transform storage buffer ===
    {
        dm_storage_buffer_desc b_desc = { 0 };
        b_desc.write        = true;
        b_desc.size         = MAX_ENTITIES * sizeof(transform);
        b_desc.stride       = sizeof(transform);
        b_desc.data         = app_data->entities.transforms;

        if(!dm_renderer_create_storage_buffer(b_desc, &app_data->entities.transform_sb, context)) return false;
    }

    // === physics storage buffer ===
    {
        dm_storage_buffer_desc b_desc = { 0 };
        b_desc.write        = true;
        b_desc.size         = MAX_ENTITIES * sizeof(physics);
        b_desc.stride       = sizeof(physics);
        b_desc.data         = app_data->entities.phys;

        if(!dm_renderer_create_storage_buffer(b_desc, &app_data->entities.physics_sb, context)) return false;
    }

    // === instance storage buffer ===
    {
        dm_storage_buffer_desc b_desc = { 0 };
        b_desc.write        = true;
        b_desc.size         = MAX_ENTITIES * sizeof(instance);
        b_desc.stride       = sizeof(instance);
        b_desc.data         = NULL;

        if(!dm_renderer_create_storage_buffer(b_desc, &app_data->entities.instance_sb, context)) return false;
    }

    // === material buffer ===
    {
        dm_storage_buffer_desc desc = { 0 };
        desc.size = sizeof(material) * MAX_ENTITIES;
        desc.stride = sizeof(material);
        desc.write = false;
        desc.data = app_data->entities.materials;

        if(!dm_renderer_create_storage_buffer(desc, &app_data->entities.material_sb, context)) return false;
    }

    // === compute pipeline ===
    {
        dm_compute_pipeline_desc desc = { 0 };
#ifdef DM_DIRECTX12
        dm_strcpy(desc.shader.path, "assets/update_entities.cso");
#elif defined(DM_VULKAN)
        dm_strcpy(desc.shader.path, "assets/compute_shader.spv");
#endif

        if(!dm_compute_create_compute_pipeline(desc, &app_data->entities.compute_pipeline, context)) return false;
    }

    return true;
}

void update_entities(dm_context* context)
{
    application_data* app_data = context->app_data;

    // update the transforms and compute model matrices
    app_data->entities.resources.transform_buffer   = app_data->entities.transform_sb.descriptor_index;
    app_data->entities.resources.physics_buffer     = app_data->entities.physics_sb.descriptor_index;
    app_data->entities.resources.instance_buffer    = app_data->entities.instance_sb.descriptor_index;
    app_data->entities.resources.rt_instance_buffer = app_data->entities.rt_instance_sb.descriptor_index;
    app_data->entities.resources.blas_buffer        = app_data->rt_data.blas_buffer.descriptor_index;

    dm_compute_command_bind_compute_pipeline(app_data->entities.compute_pipeline, context);
    dm_compute_command_set_root_constants(0,5,0, &app_data->entities.resources, context);
    dm_compute_command_dispatch(1024,1,1, context);
}

