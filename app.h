#ifndef __APP_H__
#define __APP_H__

#include "dm.h"

#include "entities.h"
#include "rendering/raster_pipeline.h"
#include "rendering/rt_pipeline.h"
#include "rendering/debug_pipeline.h"

#include "camera.h"

typedef struct application_data_t
{
    raster_pipeline_data raster_data;
    rt_pipeline_data     rt_data;
    debug_pipeline_data  debug_data;

    void* gui_context;
    uint8_t font16, font32;

    char       fps_text[512];
    char       frame_time_text[512];

    //transform transforms[MAX_ENTITIES];
    //physics   phys[MAX_ENTITIES];
    //dm_raytracing_instance raytracing_instances[MAX_ENTITIES];

    entity_data entities;

    simple_camera camera;

    dm_mat4 gui_proj;

    dm_timer frame_timer, fps_timer;
    uint16_t frame_count;

    bool ray_trace;
} application_data;

#endif
