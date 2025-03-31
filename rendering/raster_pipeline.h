#ifndef __RASTER_PIPELINE_H__
#define __RASTER_PIPELINE_H__

#include "dm.h"

typedef struct raster_pipeline_data_t
{
    dm_resource_handle pipeline, compute_pipeline;
    dm_resource_handle transform_sb, instance_cb;
    dm_resource_handle vb, ib, cb;
} raster_pipeline_data;

bool raster_pipeline_init(dm_context* context);
bool raster_pipeline_update(dm_context* context);
bool raster_pipeline_render(dm_context* context);

#endif
