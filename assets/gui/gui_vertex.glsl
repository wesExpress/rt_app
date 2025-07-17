#version 460 core

#include "../shader_include.h"

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

struct camera_data
{
    mat4 view_proj;
};

layout(push_constant) uniform render_resources
{
    uint camera_data_index;
    uint texture_index;
    uint sampler_index;
}; 

layout(set=0, binding=0) uniform u0{ camera_data data; } uniform_buffers[RESOURCE_HEAP_SIZE];


void main()
{
    mat4 view_projection = uniform_buffers[camera_data_index].data.view_proj;

    gl_Position = view_projection * vec4(pos, 0.f,1.f);

    out_uv    = in_uv;
    out_color = in_color;
}

