#ifndef __RASTER_PIPELINE_H__
#define __RASTER_PIPELINE_H__

#include "dm.h"

typedef struct render_resources_t
{
    uint32_t scene_cb;
    uint32_t material_buffer_index;
    uint32_t mesh_buffer_index;
    uint32_t node_buffer_index;
    uint32_t light_buffer_index;
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
    dm_resource_handle cb;

    render_resources  render_data;

    raster_scene_data scene_data;
    compute_data      c_data;
} raster_pipeline_data;

bool raster_pipeline_init(dm_context* context);
bool raster_pipeline_update(dm_context* context);
void raster_pipeline_render(dm_scene scene, dm_context* context);

#endif
