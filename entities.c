#include "entities.h"
#include "app.h"

#ifndef DM_DEBUG
DM_INLINE
#endif
transform init_transform(float world_scale, dm_context* context)
{
    transform t;

    t.position[0] = dm_random_float(context) * world_scale - world_scale * 0.5f;
    t.position[1] = dm_random_float(context) * world_scale - world_scale * 0.5f;
    t.position[2] = dm_random_float(context) * world_scale - world_scale * 0.5f;

    //float scaling = dm_random_float(context) * 30.f;
    float scaling = 1.f;
    t.scale[0] = scaling;
    t.scale[1] = scaling;
    t.scale[2] = scaling;

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

    return p;
}

bool init_entities(dm_context* context)
{
    application_data* app_data = context->app_data;

    for(uint32_t i=0; i<MAX_ENTITIES; i++)
    {
        app_data->entities.transforms[i] = init_transform(75.f, context);
        app_data->entities.phys[i]       = init_physics(context);
    }

    return true;
}

bool init_entity_pipeline(dm_context* context)
{
    application_data* app_data = context->app_data;

    // === transform storage buffer ===
    {
        dm_storage_buffer_desc b_desc = { 0 };
        b_desc.size         = MAX_ENTITIES * sizeof(transform);
        b_desc.stride       = sizeof(transform);
        b_desc.data         = app_data->entities.transforms;

        if(!dm_renderer_create_storage_buffer(b_desc, &app_data->entities.transform_sb, context)) return false;
    }

    // === physics storage buffer ===
    {
        dm_storage_buffer_desc b_desc = { 0 };
        b_desc.size         = MAX_ENTITIES * sizeof(physics);
        b_desc.stride       = sizeof(physics);
        b_desc.data         = app_data->entities.phys;

        if(!dm_renderer_create_storage_buffer(b_desc, &app_data->entities.physics_sb, context)) return false;
    }

    // === mesh buffer ===
    {
        dm_storage_buffer_desc desc = { 0 };
        desc.size = sizeof(mesh) * MAX_ENTITIES;
        desc.stride = sizeof(mesh);
        desc.data = app_data->entities.meshes;

        if(!dm_renderer_create_storage_buffer(desc, &app_data->entities.mesh_sb, context)) return false;
    }

    return true;
}

void update_entities(dm_context* context)
{
    application_data* app_data = context->app_data;

    for(uint32_t i=0; i<MAX_ENTITIES; i++)
    {
        transform t = app_data->entities.transforms[i];
        physics   p = app_data->entities.phys[i];

        dm_mat4 translation, rotation, scaling, model;
    
        dm_mat_translate_make(t.position, translation);
        dm_mat4_rotate_from_quat(t.orientation, rotation);
        dm_mat_scale_make(t.scale, scaling);

        dm_mat4_identity(model);
        dm_mat4_mul_mat4(model, scaling, model);
        dm_mat4_mul_mat4(model, rotation, model);
        dm_mat4_mul_mat4(model, translation, model);

        dm_vec3 delta_rot;
        dm_quat delta_quat;
        dm_vec3_scale(p.w, context->delta, delta_rot);
        dm_vec3_mul_quat(delta_rot, t.orientation, delta_quat);
        dm_quat_add_quat(t.orientation, delta_quat, t.orientation);
        dm_quat_norm(t.orientation, t.orientation);

        app_data->entities.transforms[i] = t;

#ifdef DM_DIRECTX12
        dm_mat4_transpose(model, model);
        dm_memcpy(app_data->raster_data.instances[i].model, model, sizeof(model));
        dm_memcpy(app_data->entities.rt_instances[i].transform, model, sizeof(float) * 3 * 4);
#elif defined(DM_VULKAN)
        dm_memcpy(app_data->raster_data.instances[i].model, model, sizeof(model));
        dm_mat4_transpose(model, model);
        dm_memcpy(app_data->entities.rt_instances[i].transform, model, sizeof(float) * 3 * 4);
#endif

        uint8_t mesh_index = 0;
        uint8_t material_index = 0;

        app_data->entities.rt_instances[i].blas_address = app_data->rt_data.blas_addresses[mesh_index];
        app_data->entities.rt_instances[i].mask         = 0xFF;
        app_data->entities.rt_instances[i].id           = i;

        app_data->entities.meshes[i].vb_index       = app_data->meshes[mesh_index].vb.descriptor_index;
        app_data->entities.meshes[i].ib_index       = app_data->meshes[mesh_index].ib.descriptor_index;
        app_data->entities.meshes[i].material_index = material_index;
    }

    dm_render_command_update_storage_buffer(app_data->entities.rt_instances, sizeof(app_data->entities.rt_instances), app_data->entities.rt_instance_sb, context);
    dm_render_command_update_storage_buffer(app_data->entities.meshes, sizeof(app_data->entities.meshes), app_data->entities.mesh_sb, context);
    dm_render_command_update_vertex_buffer(app_data->raster_data.instances, sizeof(app_data->raster_data.instances), app_data->raster_data.inst_vb, context);
}

