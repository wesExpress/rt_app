#version 460

#include "../shader_include.h"

layout(location = 0) in vec2 pos;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 out_color;

struct camera_data
{
    mat4 proj;
};

layout(push_constant) uniform render_resources 
{
    uint camera_data_index;
};

layout(set=0, binding=0) uniform u0{ camera_data data; } uniform_buffers[RESOURCE_HEAP_SIZE];

void main()
{
    mat4 projection = uniform_buffers[camera_data_index].data.proj;

    gl_Position = projection * vec4(pos.x, pos.y, 0.f, 1.f);

    out_color = in_color;
}

