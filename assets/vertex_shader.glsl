#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void main()
{
    gl_Position.x = position.x;
    gl_Position.y = -position.y;
    gl_Position.z = position.z;
    gl_Position.w = 1.f;

    out_color = in_color;
}

