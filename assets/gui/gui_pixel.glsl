#version 450

layout(location = 0) in vec2 out_uv;
layout(location = 1) in vec4 out_color;

layout(location = 0) out vec4 frag_color;

layout(set = 1, binding = 0) uniform sampler2D texture_sampler;

void main()
{
    frag_color = texture(texture_sampler, out_uv) * out_color;
    //frag_color = vec4(1,0,0,1);
}

