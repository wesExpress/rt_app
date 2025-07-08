#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 out_uv;
layout(location = 1) in vec4 out_color;

layout(location = 0) out vec4 frag_color;

layout(set=1, binding=0) uniform sampler2D texture_sampler[100];

layout(push_constant) uniform render_resources
{
    uint scene_cb;
    uint font_texture;
}; 

void main()
{
    frag_color = texture(texture_sampler[font_texture], out_uv) * out_color;
}

