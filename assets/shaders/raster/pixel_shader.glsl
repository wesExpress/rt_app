#version 460

#include "../shader_include.h"

layout(location = 0) in vec4 world_pos;
layout(location = 1) in vec2 fragment_tex_coords;
layout(location = 2) in mat3 tbn;
layout(location = 5) in flat int instance_index;

layout(location = 0) out vec4 pixel_color;

struct material
{
    uint vb_index;
    uint ib_index;
    uint is_indexed;
    uint material_indices[3];
    uint sampler_index;
    uint padding;
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
    vec3 light = vec3(0,0,0);

    material m = materials[material_buffer_index].data[instance_index];

    vec3 diffuse_color = texture(sampler2D(textures[m.material_indices[0]], samplers[m.sampler_index]), fragment_tex_coords).rgb; 
    vec3 normal_map_normal = texture(sampler2D(textures[m.material_indices[1]], samplers[m.sampler_index]), fragment_tex_coords).rgb;

    normal_map_normal = normalize(normal_map_normal * 2 - 1);
    vec3 normal = normalize(tbn * normal_map_normal);
    vec3 dir    = normalize(light - world_pos.xyz);

    float diff = max(dot(normal, dir), 0);

    pixel_color = vec4(diffuse_color * diff, 1);
}

