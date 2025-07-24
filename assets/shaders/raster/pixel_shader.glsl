#version 460

#include "../shader_include.h"
#define float3 vec3
#include "../lighting.h"

layout(location = 0) in vec4 world_pos;
layout(location = 1) in vec2 fragment_tex_coords;
layout(location = 2) in mat3 tbn;
layout(location = 5) in flat int instance_index;

layout(location = 0) out vec4 pixel_color;

struct material
{
    uint diffuse_texture_index;
    uint metallic_texture_index;
    uint normal_map_index;
    uint specular_map_index;
    uint occlusion_map_index;
    uint emissive_map_index;

    uint diffuse_sampler_index;
    uint metallic_sampler_index;
    uint normal_sampler_index;
    uint specular_sampler_index;
    uint occlusion_sampler_index;
    uint emissive_sampler_index;

    uint padding[2];
};

struct mesh_data
{
    uint vb_index, ib_index;
    uint material_index;
    uint padding;
};

struct camera_data
{
    mat4 view_proj;
};

layout(push_constant) uniform render_resources
{
    uint camera_data_index;
    uint material_buffer_index;
    uint mesh_buffer_index;
};

layout(set=0, binding=0) uniform u0 { camera_data data; } scene_uniforms[RESOURCE_HEAP_SIZE];
layout(std140, set=0, binding=0) buffer b0 { material data[]; }  materials[RESOURCE_HEAP_SIZE];
layout(std140, set=0, binding=0) buffer b1 { mesh_data data[]; } meshes[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) uniform texture2D textures[RESOURCE_HEAP_SIZE];
layout(set=1, binding=0) uniform sampler   samplers[SAMPLER_HEAP_SIZE];

void main()
{
    vec3 light = vec3(0,0,0);

    mat4 view_proj = scene_uniforms[camera_data_index].data.view_proj;
    mesh_data mesh = meshes[mesh_buffer_index].data[instance_index];
    material  mat  = materials[material_buffer_index].data[mesh.material_index];

    vec3 diffuse_color      = texture(sampler2D(textures[mat.diffuse_texture_index],  samplers[mat.diffuse_sampler_index]),   fragment_tex_coords).rgb; 
    vec3 metallic_roughness = texture(sampler2D(textures[mat.metallic_texture_index], samplers[mat.metallic_sampler_index]),  fragment_tex_coords).rgb;
    vec3 normal_map_normal  = texture(sampler2D(textures[mat.normal_map_index],       samplers[mat.normal_sampler_index]),    fragment_tex_coords).rgb;
    vec3 occlusion          = texture(sampler2D(textures[mat.occlusion_map_index],    samplers[mat.occlusion_sampler_index]), fragment_tex_coords).rgb;
    vec3 emission           = texture(sampler2D(textures[mat.emissive_map_index],     samplers[mat.emissive_sampler_index]),  fragment_tex_coords).rgb;

    float metallic  = metallic_roughness.b;
    float roughness = metallic_roughness.g;

    normal_map_normal = normalize(normal_map_normal * 2 - 1);
    vec3 normal = normalize(tbn * normal_map_normal);

    // lighting
    vec3 light_pos = { 0,0,0 };
    vec3 light_color = { 1,1,1 };
    vec3 light_ambient = { 0,0,0 };

    vec3 color = calculate_lighting(world_pos.xyz, normal, light_pos, light_color, light_ambient, diffuse_color, view_proj[3].xyz, roughness, metallic);

    pixel_color = vec4(light_ambient * occlusion + color + emission, 1);
}

