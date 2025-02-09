struct PS_INPUT
{
    float4 position : SV_Position;
    float4 color    : COLOR1;
};

struct PS_OUTPUT
{
    float4 color : SV_Target;
};

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;

    output.color = input.color;

    return output;
}
