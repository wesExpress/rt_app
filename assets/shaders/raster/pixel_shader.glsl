#version 450

layout(location = 0) in  vec3 fragment_normal;
layout(location = 1) in  vec4 fragment_color;

layout(location = 0) out vec4 pixel_color;

void main()
{
    pixel_color = fragment_color;
}

