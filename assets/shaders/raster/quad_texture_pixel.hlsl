struct fragment_in
{
    float4 position   : SV_Position;
    float2 tex_coords : TEX_COORDS1;
};

struct render_resources
{
    uint texture_index;
    uint sampler_index;
};

ConstantBuffer<render_resources> resources : register(b0);

float4 main(fragment_in f_in) : SV_Target
{
    Texture2D    texture = ResourceDescriptorHeap[resources.texture_index];
    SamplerState sampler = SamplerDescriptorHeap[resources.sampler_index];

    return texture.Sample(sampler, f_in.tex_coords);
}
