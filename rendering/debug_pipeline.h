#ifndef __DEBUG_PIPELINE_H__
#define __DEBUG_PIPELINE_H__

#include "dm.h"

typedef struct debug_vertex_t
{
    dm_vec4 position;
    dm_vec4 color;
} debug_vertex;

#define DEBUG_MAX_VERTICES 10000
typedef struct debug_pipeline_data_t
{
    dm_resource_handle pipeline;
    dm_resource_handle vb, ib;

    debug_vertex vertices[DEBUG_MAX_VERTICES];
    uint32_t     indices[DEBUG_MAX_VERTICES * 3];
    uint32_t     vertex_count, index_count;
} debug_pipeline_data;

bool debug_pipeline_init(dm_context* context);

void debug_pipeline_draw_line(dm_vec3 origin, dm_vec3 direction, float length, dm_vec4 color, dm_context* context);

bool debug_pipeline_update(dm_context* context);
bool debug_pipeline_render(dm_context* context);

#endif
