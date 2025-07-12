struct PS_INPUT
{
    float4 pos   : SV_Position;
    float2 uv    : TEX_COORDS1;
    float4 color : COLOR1;
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
    SamplerState sampler   = SamplerDescriptorHeap[resources.sampler_index];

    float4 tex_color = font_texture.Sample(sampler, input.uv);

    return input.color * tex_color;
}
