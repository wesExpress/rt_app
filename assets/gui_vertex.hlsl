struct VS_INPUT
{
    float2 pos   : POSITION;
    float2 uv    : TEX_COORDS0;
    float4 color : COLOR0;
};

struct VS_OUTPUT
{
    float4 pos   : SV_Position;
    float2 uv    : TEX_COORDS1;
    float4 color : COLOR1;
};

cbuffer ConstantBuffer : register(b0)
{
    matrix proj;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    output.pos   = float4(input.pos, 0, 1.f);
    output.pos   = mul(output.pos, proj);
    output.uv    = input.uv;
    output.color = input.color;

    return output;
}
