#ifndef __RT_PIPELINE_H__
#define __RT_PIPELINE_H__

#include "dm.h"
#include "../entities.h"

typedef struct scene_cb_t
{
    dm_mat4 inv_view, inv_proj;
    dm_vec4 origin;
} scene_cb;

typedef struct rt_resources_t
{
    uint32_t acceleration_structure;
    uint32_t image;
    uint32_t constant_buffer;
    uint32_t material_buffer;
} rt_resources;

typedef struct rt_pipeline_data_t
{
    dm_resource_handle pipeline;
    dm_resource_handle cube_blas, triangle_blas, quad_blas;
    dm_resource_handle tlas;
    dm_resource_handle vb, ib, cb;
    dm_resource_handle image;
    dm_resource_handle material_buffer;

    size_t blas_addresses[DM_TLAS_MAX_BLAS];
    material materials[MAX_ENTITIES];

    scene_cb scene_data;

    rt_resources resources;
} rt_pipeline_data;

bool rt_pipeline_init(dm_context* context);
bool rt_pipeline_update(dm_context* context);
bool rt_pipeline_render(dm_context* context);

#endif
