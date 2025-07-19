struct VERTEX_IN
{
    float4 pos      : POSITION;
    float4 color    : COLOR;
};

struct VERTEX_OUT
{
    float4 pos    : SV_POSITION;
    float4 color  : COLOR1;
};

struct scene_data
{
    matrix view_proj;
};

struct render_resources 
{
    uint scene_cb;
};

ConstantBuffer<render_resources> resources : register(b0);

VERTEX_OUT main(VERTEX_IN v_in)
{
    VERTEX_OUT v_out;

    ConstantBuffer<scene_data> scene_cb = ResourceDescriptorHeap[resources.scene_cb];

    v_out.pos = float4(v_in.pos.x, v_in.pos.y, v_in.pos.z, 1.f);
    v_out.pos = mul(v_out.pos, scene_cb.view_proj);

    v_out.color = v_in.color;

    return v_out;
}
