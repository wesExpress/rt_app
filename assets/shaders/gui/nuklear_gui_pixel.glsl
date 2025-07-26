#version 460

#include "../shader_include.h"

layout(location=0) in vec2 fragment_tex_coords;
layout(location=1) in vec4 fragment_color;

layout(location=0) out vec4 pixel_color;

layout(push_constant) uniform resource_indices
{
    uint constant_buffer_index;
    uint font_texture_index;
    uint sampler_index;
};

layout(set=0, binding=0) uniform texture2D textures[RESOURCE_HEAP_SIZE];
layout(set=1, binding=0) uniform sampler   samplers[SAMPLER_HEAP_SIZE];

void main()
{
    pixel_color = texture(sampler2D(textures[font_texture_index], samplers[sampler_index]), fragment_tex_coords) * fragment_color;
}

