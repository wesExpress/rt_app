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
    uint vb_index;
    uint ib_index;
    uint is_indexed;
    uint material_indices[3];
    uint sampler_index;
    uint padding;
};

struct render_resources 
{
    uint scene_cb;
    uint material_buffer_index;
};

ConstantBuffer<render_resources> resources : register(b0);

float4 main(FRAGMENT_IN frag_in) : SV_Target
{
    float3 light = float3(0,0,0);
    float3 dir  = normalize(light - frag_in.world_pos.xyz);

    StructuredBuffer<material> materials = ResourceDescriptorHeap[resources.material_buffer_index];

    material m = materials[frag_in.instance_index];

    Texture2D    diffuse_texture = ResourceDescriptorHeap[m.material_indices[0]];
    Texture2D    normal_map      = ResourceDescriptorHeap[m.material_indices[1]];
    SamplerState sampler = SamplerDescriptorHeap[m.sampler_index];

    float3 diffuse_color = diffuse_texture.Sample(sampler, frag_in.tex_coords).rgb;
    float3 normal        = normal_map.Sample(sampler, frag_in.tex_coords).rgb;
    normal = normalize(normal * 2 - 1);
    normal = normalize(mul(normal, frag_in.tbn));
    
    float diff = max(dot(normal, dir), 0);

    return float4(diffuse_color * diff, 1); 
}
