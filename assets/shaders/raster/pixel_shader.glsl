#version 460

#include "../shader_include.h"

layout(location = 0) in vec2 fragment_tex_coords;
layout(location = 1) in flat int instance_index;

layout(location = 0) out vec4 pixel_color;


struct material
{
    uint vb_index;
    uint ib_index;
    uint is_indexed;
    uint diffuse_texture_index;
    uint sampler_index;
};

layout(push_constant) uniform render_resources
{
    uint camera_data_index;
    uint material_buffer_index;
};

layout(set=0, binding=0) buffer b0 { material data[]; } materials[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) uniform texture2D textures[RESOURCE_HEAP_SIZE];
layout(set=1, binding=0) uniform sampler   samplers[SAMPLER_HEAP_SIZE];

void main()
{
    material m = materials[material_buffer_index].data[instance_index];
    pixel_color = texture(sampler2D(textures[m.diffuse_texture_index], samplers[m.sampler_index]), fragment_tex_coords);
}

