#version 460

#include "../shader_include.h"

layout(location = 0) in vec4 vertex_position_u;
layout(location = 1) in vec4 vertex_normal_v;
layout(location = 2) in mat4 model;

layout(location = 0) out vec2 fragment_tex_coords;

struct instance
{
    mat4 model;
};

struct camera_data
{
    mat4 view_proj;
};

layout(std140, set=0, binding=0) uniform u0 { camera_data data; } uniform_buffers[RESOURCE_HEAP_SIZE];

layout(push_constant) uniform render_resources 
{
    uint camera_data_index;
    uint sampler_index;
    uint diffuse_texture_index;
};

void main()
{
    mat4 view_projection = uniform_buffers[camera_data_index].data.view_proj;

    gl_Position = view_projection * model * vec4(vertex_position_u.xyz, 1);

    fragment_tex_coords = vec2(vertex_position_u.w, vertex_normal_v.w);
}

