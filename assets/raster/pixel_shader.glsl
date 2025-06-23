#version 450

layout(location = 0) in  vec3 out_normal;
layout(location = 1) in  vec4 out_color;

layout(location = 0) out vec4 frag_color;

void main()
{
    frag_color = out_color;
}

