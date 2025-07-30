#include "../lighting.h"

struct FRAGMENT_IN 
{
    float4 fragment_pos : SV_POSITION;
    float4 world_pos    : POSITION1;
    float2 tex_coords   : TEX_COORDS1;
    float3x3 tbn        : TBN;
    uint instance_index : INST;
};

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

struct node_data
{
    uint   mesh_index, material_index;
    uint   padding[2];
    matrix model;
};

struct camera_data
{
    matrix view_proj;
};

struct light_source
{
    float4 position_str;
    float4 color;
};

struct render_resources 
{
    uint scene_cb;
    uint material_buffer_index;
    uint mesh_buffer_index;
    uint node_buffer_index;
    uint light_buffer_index;
};

ConstantBuffer<render_resources> resources : register(b0);

float4 main(FRAGMENT_IN frag_in) : SV_Target
{
    ConstantBuffer<camera_data> camera    = ResourceDescriptorHeap[resources.scene_cb];
    StructuredBuffer<material> materials  = ResourceDescriptorHeap[resources.material_buffer_index];
    StructuredBuffer<node_data> nodes     = ResourceDescriptorHeap[resources.node_buffer_index];
    StructuredBuffer<light_source> lights = ResourceDescriptorHeap[resources.light_buffer_index];

    node_data node = nodes[frag_in.instance_index];
    material  mat  = materials[node.material_index];

    Texture2D    diffuse_texture = ResourceDescriptorHeap[mat.diffuse_texture_index];
    Texture2D    metallic_map    = ResourceDescriptorHeap[mat.metallic_texture_index];
    Texture2D    normal_map      = ResourceDescriptorHeap[mat.normal_map_index];
    Texture2D    occlusion_map   = ResourceDescriptorHeap[mat.occlusion_map_index];
    Texture2D    emissive_map    = ResourceDescriptorHeap[mat.emissive_map_index];

    SamplerState diffuse_sampler   = SamplerDescriptorHeap[mat.diffuse_sampler_index];
    SamplerState metallic_sampler  = SamplerDescriptorHeap[mat.metallic_sampler_index];
    SamplerState normal_sampler    = SamplerDescriptorHeap[mat.normal_sampler_index];
    SamplerState occlusion_sampler = SamplerDescriptorHeap[mat.occlusion_sampler_index];
    SamplerState emissive_sampler  = SamplerDescriptorHeap[mat.emissive_sampler_index];

    // pbr values
    float3 diffuse_color      = diffuse_texture.Sample(diffuse_sampler, frag_in.tex_coords).rgb;
    float3 metallic_roughness = metallic_map.Sample(metallic_sampler, frag_in.tex_coords).rgb;
    float3 occlusion          = occlusion_map.Sample(occlusion_sampler, frag_in.tex_coords).rgb;
    float3 emission           = emissive_map.Sample(emissive_sampler, frag_in.tex_coords).rgb;

    float metallic  = metallic_roughness.b;
    float roughness = metallic_roughness.g;

    float3 normal = normal_map.Sample(normal_sampler, frag_in.tex_coords).rgb;
    normal        = normalize(normal * 2 - 1);
    normal        = normalize(mul(normal, frag_in.tbn));

    // lighting
    light_source light = lights[0];

    float3 color = calculate_lighting(frag_in.world_pos.xyz, normal, light.position_str.xyz, light.color.rgb, diffuse_color, transpose(camera.view_proj)[3].xyz, roughness, metallic);

    // final color
    return float4(color + emission, 1);
}
