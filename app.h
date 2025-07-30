#ifndef __APP_H__
#define __APP_H__

#include "dm.h"

//#include "entities.h"
#include "rendering/raster_pipeline.h"
#include "rendering/rt_pipeline.h"
#include "rendering/debug_pipeline.h"
#include "rendering/quad_texture.h"
#include "rendering/nuklear_gui.h"

#include "camera.h"

typedef struct material_t
{
    uint32_t diffuse_texture_index;
    uint32_t metallic_roughness_index;
    uint32_t normal_map_index;
    uint32_t specular_map_index;
    uint32_t occlusion_map_index;
    uint32_t emissive_map_index;

    uint32_t diffuse_sampler_index;
    uint32_t metallic_sampler_index;
    uint32_t normal_sampler_index;
    uint32_t specular_sampler_index;
    uint32_t occlusion_sampler_index;
    uint32_t emissive_sampler_index;

    uint32_t padding[2];
} material;

typedef struct light_source_t
{
    dm_vec4 position_str;
    dm_vec4 color;
} light_source;

typedef struct mesh_t
{
    uint32_t vb_index, ib_index;
    uint32_t padding[2];
} mesh;

typedef struct application_data_t
{
    //entity_data          entities;
    raster_pipeline_data raster_data;
    rt_pipeline_data     rt_data;
    debug_pipeline_data  debug_data;
    quad_texture_data    quad_texture_data;
    void* nuklear_data;
    nuklear_context nk_context;

    light_source lights[10];
    dm_resource_handle light_buffer;

    dm_resource_handle node_buffer;

    void* gui_context;
    uint8_t font16, font32;

    char fps_text[512];
    char frame_time_text[512];
    char gpu_time_text[512];

    dm_scene sponza_scene;

    material* materials;
    mesh*     meshes;
    dm_resource_handle material_sb, mesh_sb;
    uint32_t material_count;

    dm_resource_handle white_texture, black_texture;
    dm_resource_handle default_sampler;

    simple_camera camera;

    dm_timer frame_timer, fps_timer;
    uint16_t frame_count;

    struct nk_colorf clear_color;

    bool ray_trace;
} application_data;

#endif
