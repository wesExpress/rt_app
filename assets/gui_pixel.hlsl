struct PS_INPUT
{
    float4 pos   : SV_Position;
    float2 uv    : TEX_COORDS1;
    float4 color : COLOR1;
};

SamplerState sample_state;
Texture2D    font_texture : register(t0);

float4 main(PS_INPUT input) : SV_Target
{
    float4 tex_color = font_texture.Sample(sample_state, input.uv);

    return input.color;
}
