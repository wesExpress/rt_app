#ifndef __RT_PIPELINE_H__
#define __RT_PIPELINE_H__

#include "dm.h"

typedef struct scene_cb_t
{
    dm_vec4 sky_color;
} scene_cb;

typedef struct rt_resources_t
{
    uint32_t acceleration_structure;
    uint32_t image, random_image;
    uint32_t camera_data_index;
    uint32_t scene_data_index;
    uint32_t material_buffer_index;
    uint32_t mesh_buffer_index;
} rt_resources;

typedef struct rt_camera_data_t
{
    dm_mat4 inv_view, inv_proj;
    dm_vec4 position;
} rt_camera_data;

typedef struct rt_pipeline_data_t
{
    dm_resource_handle pipeline;
    dm_resource_handle tlas;
    dm_resource_handle scene_cb, camera_cb;
    dm_resource_handle image, random_image;
    dm_resource_handle blas[10];

    size_t blas_addresses[10];

    scene_cb scene_data;
    rt_camera_data camera_data;

    rt_resources resources;
        
    uint32_t image_width, image_height;
} rt_pipeline_data;

bool rt_pipeline_init(dm_context* context);
bool rt_pipeline_update(dm_context* context);
void rt_pipeline_render(dm_context* context);

#endif
