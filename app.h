#ifndef __APP_H__
#define __APP_H__

#include "dm.h"
#include "rendering/raster_pipeline.h"
#include "rendering/rt_pipeline.h"
#include "camera.h"

#define MAX_INSTANCES 10000
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

typedef struct application_data_t
{
    raster_pipeline_data raster_data;
    rt_pipeline_data     rt_data;

    void* gui_context;
    uint8_t font16, font32;

    char       fps_text[512];
    char       frame_time_text[512];

    transform transforms[MAX_INSTANCES];
    instance  instances[MAX_INSTANCES];
    dm_raytracing_instance raytracing_instances[MAX_INSTANCES];

    simple_camera camera;

    dm_mat4 gui_proj;

    dm_timer frame_timer, fps_timer;
    uint16_t frame_count;

    bool ray_trace;
} application_data;

#endif
