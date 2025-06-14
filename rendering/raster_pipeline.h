#ifndef __RASTER_PIPELINE_H__
#define __RASTER_PIPELINE_H__

#include "dm.h"

typedef struct compute_resources_t 
{
    uint32_t transform_buffer;
    uint32_t instance_buffer;
    uint32_t scene_cb;
} compute_resources;

typedef struct render_resources_t
{
    uint32_t scene_cb;
    uint32_t instance_buffer; 
} render_resources;

typedef struct raster_scene_data_t
{
    dm_mat4 view_proj;
} raster_scene_data;

typedef struct compute_data_t
{
    double delta_t;
} compute_data;

typedef struct raster_pipeline_data_t
{
    dm_resource_handle pipeline, compute_pipeline;
    dm_resource_handle transform_sb, instance_cb;
    dm_resource_handle vb_cube, ib_cube;
    dm_resource_handle vb_tri, ib_tri;
    dm_resource_handle vb_quad, ib_quad;
    dm_resource_handle cb, compute_cb;

    compute_resources compute_data;
    render_resources  render_data;

    raster_scene_data scene_data;
    compute_data      c_data;
} raster_pipeline_data;

bool raster_pipeline_init(dm_context* context);
bool raster_pipeline_update(dm_context* context);
bool raster_pipeline_render(dm_context* context);

#endif
