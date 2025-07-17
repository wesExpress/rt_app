#ifndef __QUAD_TEXTURE_H__
#define __QUAD_TEXTURE_H__

#include "dm.h"

typedef struct quad_texture_resources_t
{
    uint32_t image_index;
    uint32_t sampler_index;
} quad_texture_resources;

typedef struct quad_texture_data_t
{
    dm_resource_handle pipeline;
    dm_resource_handle vb, ib;

    quad_texture_resources resources;
} quad_texture_data;

bool quad_texture_init(dm_context* context);
void quad_texture_render(dm_resource_handle texture, dm_context* context);

#endif

