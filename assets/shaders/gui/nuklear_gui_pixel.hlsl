struct fragment
{
    float4 position   : SV_Position;
    float2 tex_coords : TEX_COORDS1;
    float4 color      : COLOR1;
};

struct resource_indices
{
    uint camera_data_index;
    uint texture_index;
    uint sampler_index;
};

ConstantBuffer<resource_indices> resources : register(b0);

float4 main(fragment f) : SV_Target
{
    Texture2D font_texture = ResourceDescriptorHeap[resources.texture_index];
    SamplerState s         = SamplerDescriptorHeap[resources.sampler_index];

    return font_texture.Sample(s, f.tex_coords) * f.color;
    //return f.color;
}
