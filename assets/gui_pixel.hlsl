struct PS_INPUT
{
    float4 pos   : SV_Position;
    float2 uv    : TEX_COORDS1;
    float4 color : COLOR1;
};

float4 main(PS_INPUT input) : SV_Target
{
    return input.color;
}
