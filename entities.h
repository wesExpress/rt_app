#ifndef __ENTITIES_H__
#define __ENTITIES_H__

#include "dm.h"

#define MAX_ENTITIES 100

typedef struct transform_t
{
    dm_vec4 position;
    dm_vec4 scale;
    dm_quat orientation;
} transform;

typedef struct instance_t 
{
    dm_mat4 model;
    dm_mat4 normal;
} instance;

typedef struct material_t
{
    uint32_t vb_index;
    uint32_t ib_index;
} material;

#endif
