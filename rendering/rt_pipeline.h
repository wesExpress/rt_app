#ifndef __RT_PIPELINE_H__
#define __RT_PIPELINE_H__

#include "dm.h"

typedef struct rt_pipeline_data_t
{
    dm_resource_handle pipeline, compute_pipeline;
    dm_resource_handle acceleration_structure;
    dm_resource_handle vb, ib;

    size_t blas_addresses[DM_TLAS_MAX_BLAS];
} rt_pipeline_data;

bool rt_pipeline_init(dm_resource_handle vb, dm_resource_handle ib, dm_context* context);
bool rt_pipeline_update(dm_context* context);
bool rt_pipeline_render(dm_context* context);

#endif
