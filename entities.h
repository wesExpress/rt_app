#ifndef __ENTITIES_H__
#define __ENTITIES_H__

#include "dm.h"

#define MAX_ENTITIES (8 << 8) 

typedef struct transform_t
{
    dm_vec4 position;
    dm_vec4 scale;
    dm_quat orientation;
} transform;

typedef struct material_t
{
    uint32_t vb_index;
    uint32_t ib_index;
    uint32_t is_indexed;
    uint32_t diffuse_texture_index;
    uint32_t normal_map_index;
    uint32_t sampler_index;
    uint32_t padding[2];
} material;

typedef struct physics_t
{
    dm_vec4 w;
} physics;

typedef struct compute_resources_t 
{
    uint32_t transform_buffer;
    uint32_t physics_buffer;
    uint32_t instance_buffer;
    uint32_t rt_instance_buffer;
    uint32_t blas_buffer;
} compute_resources;

typedef struct entity_data
{
    transform transforms[MAX_ENTITIES];
    dm_raytracing_instance rt_instances[MAX_ENTITIES];
    physics   phys[MAX_ENTITIES];
    material  materials[MAX_ENTITIES];

    dm_resource_handle transform_sb, instance_sb, rt_instance_sb, physics_sb, material_sb;

    compute_resources resources;
} entity_data;

bool init_entities(dm_context* context);
bool init_entity_pipeline(dm_context* context);
void update_entities(dm_context* context);

#endif
