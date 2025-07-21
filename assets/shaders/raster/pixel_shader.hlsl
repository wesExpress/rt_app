struct FRAGMENT_IN 
{
    float4 pos        : SV_POSITION;
    float2 tex_coords : TEX_COORDS1;
    uint instance_index : INST;
};

struct material
{
    uint vb_index;
    uint ib_index;
    uint is_indexed;
    uint diffuse_texture_index;
    uint sampler_index;
};

struct render_resources 
{
    uint scene_cb;
    uint material_buffer_index;
};

ConstantBuffer<render_resources> resources : register(b0);

float4 main(FRAGMENT_IN frag_in) : SV_Target
{
    StructuredBuffer<material> materials = ResourceDescriptorHeap[resources.material_buffer_index];

    material m = materials[frag_in.instance_index];

    Texture2D    texture = ResourceDescriptorHeap[m.diffuse_texture_index];
    SamplerState sampler = SamplerDescriptorHeap[m.sampler_index];

    return texture.Sample(sampler, frag_in.tex_coords);
}
