#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 out_color;

layout(set=0, binding=0) uniform uniform_buffer 
{
    mat4x4 ortho_proj;
} uniform_buffers[100];

layout(push_constant) uniform render_resources 
{
    uint scene_cb;
};

void main()
{
    mat4 projection = uniform_buffers[scene_cb].ortho_proj;

    gl_Position = projection * vec4(pos.x, pos.y, 0.f, 1.f);

    out_color = in_color;
}

