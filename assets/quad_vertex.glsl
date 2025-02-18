#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 out_color;

layout(set=0, binding=0) uniform UniformBufferObject
{
    mat4x4 view_projection;
} ubo;

void main()
{
    gl_Position = ubo.view_projection * vec4(pos, 0.f, 1.f);

    out_color = in_color;
}

