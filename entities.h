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

typedef struct physics_t
{
    dm_vec4 w;
} physics;

typedef struct mesh_t
{
    uint32_t vb_index, ib_index;
    uint32_t material_index;
    uint32_t padding;
} mesh;

typedef struct entity_data
{
    transform              transforms[MAX_ENTITIES];
    dm_raytracing_instance rt_instances[MAX_ENTITIES];
    physics                phys[MAX_ENTITIES];
    mesh                   meshes[MAX_ENTITIES];

    dm_resource_handle transform_sb, rt_instance_sb, physics_sb, mesh_sb;
} entity_data;

bool init_entities(dm_context* context);
bool init_entity_pipeline(dm_context* context);
void update_entities(dm_context* context);

#endif
