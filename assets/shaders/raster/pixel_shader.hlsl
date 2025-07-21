struct FRAGMENT_IN 
{
    float4 pos        : SV_POSITION;
    float2 tex_coords : TEX_COORDS1;
};

struct render_resources 
{
    uint scene_cb;
    uint sampler_index;
    uint diffuse_texture_index;
};

ConstantBuffer<render_resources> resources : register(b0);

float4 main(FRAGMENT_IN frag_in) : SV_Target
{
    Texture2D    texture = ResourceDescriptorHeap[resources.diffuse_texture_index];
    SamplerState sampler = SamplerDescriptorHeap[resources.sampler_index];

    return texture.Sample(sampler, frag_in.tex_coords);
}
