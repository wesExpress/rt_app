struct PS_INPUT
{
    float4 pos   : SV_Position;
    float2 uv    : TEX_COORDS1;
    float4 color : COLOR1;
};

SamplerState texture_sampler : register(s0);
Texture2D    font_texture    : register(t0);

float4 main(PS_INPUT input) : SV_Target
{
    float4 tex_color = font_texture.Sample(texture_sampler, input.uv);

    return input.color * tex_color;
}
