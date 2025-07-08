#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec4 vertex_color;

layout(location = 0) out vec3 fragment_normal;
layout(location = 1) out vec4 fragment_color;

struct instance
{
    mat4 model;
};

layout(std140, set=0, binding=0) uniform scene_constant_buffer
{
    mat4 view_projection;
} uniform_buffers[100];

layout(std430, set=0, binding=1) buffer instance_buffer 
{
    instance instances[];
} instance_buffers[100];

layout(push_constant) uniform render_resources 
{
    uint scene_cb;
    uint inst_b;
};

void main()
{
    vec4 p = vec4(position, 1.f);
    
    instance inst        = instance_buffers[inst_b].instances[gl_InstanceIndex];
    mat4 view_projection = uniform_buffers[scene_cb].view_projection;

    gl_Position = view_projection * inst.model * p;

    fragment_color = vertex_color;
}

