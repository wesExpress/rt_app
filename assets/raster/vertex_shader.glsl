#version 460

#include "../shader_include.h"

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 vertex_normal;
layout(location = 2) in vec4 vertex_color;

layout(location = 0) out vec3 fragment_normal;
layout(location = 1) out vec4 fragment_color;

struct instance
{
    mat4 model;
};

struct camera_data
{
    mat4 view_proj;
};

layout(std140, set=0, binding=0) uniform u0 { camera_data data; }   uniform_buffers[RESOURCE_HEAP_SIZE];
layout(std430, set=0, binding=0) buffer  b0 { instance    data[]; } instance_buffers[RESOURCE_HEAP_SIZE];

layout(push_constant) uniform render_resources 
{
    uint camera_data_index;
    uint inst_buffer_index;
};

void main()
{
    vec4 p = position;
    p.w = 1.f;
    
    instance inst        = instance_buffers[inst_buffer_index].data[gl_InstanceIndex];
    mat4 view_projection = uniform_buffers[camera_data_index].data.view_proj;

    gl_Position = view_projection * inst.model * p;

    fragment_color = vertex_color;
}

