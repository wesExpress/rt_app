#ifndef __APP_H__
#define __APP_H__

#include "dm.h"

#include "entities.h"
#include "rendering/raster_pipeline.h"
#include "rendering/rt_pipeline.h"
#include "rendering/debug_pipeline.h"
#include "rendering/quad_texture.h"

#include "camera.h"

#define MAX_MESHES 10

typedef struct application_data_t
{
    raster_pipeline_data raster_data;
    rt_pipeline_data     rt_data;
    debug_pipeline_data  debug_data;
    quad_texture_data    quad_texture_data;

    dm_resource_handle default_sampler;

    void* gui_context;
    uint8_t font16, font32;

    char       fps_text[512];
    char       frame_time_text[512];
    char gpu_time_text[512];

    entity_data entities;

    dm_mesh meshes[MAX_MESHES];
    uint16_t mesh_count;

    simple_camera camera;

    dm_timer frame_timer, fps_timer;
    uint16_t frame_count;

    bool ray_trace;
} application_data;

#endif
