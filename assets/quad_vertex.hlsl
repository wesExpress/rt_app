struct VS_INPUT
{
    float2 position : POSITION;
    float4 color    : COLOR0;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float4 color    : COLOR1;
};

cbuffer ConstantBuffer : register(b0)
{
    matrix proj;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    output.position = float4(input.position, 0, 1.f);
    output.position = mul(output.position, proj);
    output.color    = input.color;

    return output;
}
