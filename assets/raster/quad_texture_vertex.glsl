#version 460

layout(location=0) in vec2 vertex_position;
layout(location=1) in vec2 vertex_tex_coords;

layout(location=0) out vec2 fragment_tex_coords;

void main()
{
    gl_Position = vec4(vertex_position, 0, 1.f);

    fragment_tex_coords = vertex_tex_coords;
}

