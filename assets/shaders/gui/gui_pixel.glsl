#version 450

#include "../shader_include.h"

layout(location = 0) in vec2 out_uv;
layout(location = 1) in vec4 out_color;

layout(location = 0) out vec4 frag_color;

layout(push_constant) uniform render_resources
{
    uint camera_data_index;
    uint texture_index;
    uint sampler_index;
}; 

layout(set=0, binding=0) uniform texture2D textures[RESOURCE_HEAP_SIZE];
layout(set=1, binding=0) uniform sampler   samplers[SAMPLER_HEAP_SIZE];

void main()
{
    frag_color = texture(sampler2D(textures[texture_index], samplers[sampler_index]), out_uv) * out_color;
}

