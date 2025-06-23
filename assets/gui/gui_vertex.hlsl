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

struct scene_data
{
    matrix proj;
};

struct render_resources
{
    uint scene_data;
    uint texture;
};

ConstantBuffer<render_resources> resources : register(b0);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    ConstantBuffer<scene_data> scene_cb = ResourceDescriptorHeap[resources.scene_data];

    output.pos   = float4(input.pos, 0, 1.f);
    output.pos   = mul(output.pos, scene_cb.proj);
    output.uv    = input.uv;
    output.color = input.color;

    return output;
}
