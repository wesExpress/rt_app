#version 450

layout(binding = 0) uniform UniformBufferObject
{
    mat4 mvp;
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void main()
{
    vec4 p = vec4(position.x, position.y, position.z, 1.f);
    
    gl_Position = ubo.mvp * p;

    out_color = in_color;
}

