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
};

SamplerState texture_sampler : register(s0);
ConstantBuffer<render_resources> resources : register(b0);

float4 main(PS_INPUT input) : SV_Target
{
    Texture2D font_texture = ResourceDescriptorHeap[resources.texture];

    float4 tex_color = font_texture.Sample(texture_sampler, input.uv);

    return input.color * tex_color;
}
