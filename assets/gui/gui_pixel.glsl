#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 out_uv;
layout(location = 1) in vec4 out_color;

layout(location = 0) out vec4 frag_color;

layout(set=0, binding=3) uniform texture2D textures[100];
layout(set=1, binding=0) uniform sampler   samplers[100];

layout(push_constant) uniform render_resources
{
    uint scene_cb;
    uint texture_index;
    uint sampler_index;
}; 

void main()
{
    frag_color = texture(sampler2D(textures[texture_index], samplers[sampler_index]), out_uv) * out_color;
}

