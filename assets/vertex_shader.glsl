#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec4 out_color;

struct instance
{
    mat4 model;
    mat4 normal;
};

layout(std140, binding=0) uniform UniformBufferObject
{
    mat4 view_projection;
} ubo;

layout(set=0, binding=1) buffer instance_buffer 
{
    instance instances[];
};

void main()
{
    vec4 p = vec4(position.x, position.y, position.z, 1.f);
    
    gl_Position = ubo.view_projection * instances[gl_InstanceIndex].model * p;

    out_color = in_color;
}

