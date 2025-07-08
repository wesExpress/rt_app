#version 460 core
//#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

layout(set=0, binding=0) uniform uniform_buffer 
{
    mat4x4 view_projection;
} uniform_buffers[100];

layout(push_constant) uniform render_resources
{
    uint scene_cb;
    uint font_texture;
}; 

void main()
{
    mat4 view_projection = uniform_buffers[scene_cb].view_projection;

    gl_Position = view_projection * vec4(pos, 0.f,1.f);

    out_uv    = in_uv;
    out_color = in_color;
}

