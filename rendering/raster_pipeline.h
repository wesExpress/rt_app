#ifndef __RASTER_PIPELINE_H__
#define __RASTER_PIPELINE_H__

#include "dm.h"

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
    dm_resource_handle pipeline;
    dm_resource_handle cb, compute_cb;

    render_resources  render_data;

    raster_scene_data scene_data;
    compute_data      c_data;
} raster_pipeline_data;

bool raster_pipeline_init(dm_context* context);
bool raster_pipeline_update(dm_context* context);
bool raster_pipeline_render(dm_mesh mesh, dm_context* context);

#endif
