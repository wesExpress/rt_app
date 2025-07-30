#version 460

#include "../shader_include.h"

layout(location = 0) in vec4 vertex_position_u;
layout(location = 1) in vec4 vertex_normal_v;
layout(location = 2) in vec4 tangent;
layout(location = 3) in vec4 color;

layout(location = 0) out vec4 world_pos;
layout(location = 1) out vec2 fragment_tex_coords;
layout(location = 2) out mat3 tbn;
layout(location = 5) out flat int instance_index;

struct camera_data
{
    mat4 view_proj;
};

struct node_data
{
    uint mesh_index, material_index;
    uint padding[2];
    mat4 model;
};

layout(std140, set=0, binding=0) uniform u0 { camera_data data; } uniform_buffers[RESOURCE_HEAP_SIZE];
layout(std140, set=0, binding=0) buffer readonly b0 { node_data data[]; } node_buffer[RESOURCE_HEAP_SIZE];

layout(push_constant) uniform render_resources 
{
    uint camera_data_index;
    uint material_buffer_index;
    uint mesh_buffer_index;
    uint node_buffer_index;
    uint light_buffer_index;
};

void main()
{
    mat4 view_projection = uniform_buffers[camera_data_index].data.view_proj;
    
    node_data node = node_buffer[node_buffer_index].data[gl_InstanceIndex];

    world_pos   = node.model * vec4(vertex_position_u.xyz, 1);
    gl_Position = view_projection * world_pos;

    fragment_tex_coords = vec2(vertex_position_u.w, vertex_normal_v.w);

    vec3 T = normalize((node.model * tangent).xyz);
    vec3 N = normalize((node.model * vec4(vertex_normal_v.xyz, 0)).xyz);
    vec3 B = normalize(cross(N, T));

    tbn = mat3(T,B,N);

    instance_index = gl_InstanceIndex;
}

