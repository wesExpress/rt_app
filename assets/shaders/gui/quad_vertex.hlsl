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

struct scene_data
{
    matrix proj;
};

struct render_resources
{
    uint scene_cb;
};

ConstantBuffer<render_resources> resources : register(b0);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    ConstantBuffer<scene_data> scene_cb = ResourceDescriptorHeap[resources.scene_cb];

    output.position = float4(input.position, 0, 1.f);
    output.position = mul(output.position, scene_cb.proj);
    output.color    = input.color;

    return output;
}
