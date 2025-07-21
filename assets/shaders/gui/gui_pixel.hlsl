struct PS_INPUT
{
    float4 pos   : SV_Position;
    float4 color : COLOR1;
    float2 uv    : TEX_COORDS1;
};

struct render_resources
{
    uint scene_data;
    uint texture;
    uint sampler_index;
};

ConstantBuffer<render_resources> resources : register(b0);

float4 main(PS_INPUT input) : SV_Target
{
    Texture2D font_texture = ResourceDescriptorHeap[resources.texture];
    SamplerState samp      = SamplerDescriptorHeap[resources.sampler_index];

    return font_texture.Sample(samp, input.uv) * input.color;
}
