#version 460

#include "../shader_include.h"

layout(location=0) in vec2 vertex_position;
layout(location=1) in vec2 vertex_tex_coords;
layout(location=2) in vec4 vertex_color;

layout(location=0) out vec2 fragment_tex_coords;
layout(location=1) out vec4 fragment_color;

struct camera_data
{
    mat4 projection;
};

layout(push_constant) uniform render_indices
{
    uint constant_buffer_index;
    uint font_texture_index;
    uint sampler_index;
};

layout(set=0, binding=0) uniform u0 { camera_data data; } uniform_heap[RESOURCE_HEAP_SIZE];

void main()
{
    mat4 projection = uniform_heap[constant_buffer_index].data.projection;

    gl_Position = projection * vec4(vertex_position, 0, 1);

    fragment_tex_coords = vertex_tex_coords;

    fragment_color = vertex_color;
}
