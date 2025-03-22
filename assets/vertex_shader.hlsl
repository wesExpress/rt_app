struct VERTEX_IN
{
    float3 pos      : POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
};

struct VERTEX_OUT
{
    float4 pos    : SV_POSITION;
    float3 normal : NORMAL1;
    float4 color  : COLOR1;
};

cbuffer ConstantBuffer : register(b0)
{
    matrix view_proj;
};

struct inst
{
    matrix model;
    matrix obj_norm;
};

StructuredBuffer<inst> test_buffer : register(t0);

VERTEX_OUT main(VERTEX_IN v_in, uint inst_id : SV_InstanceID)
{
    VERTEX_OUT v_out;

    v_out.pos   = float4(v_in.pos.x, v_in.pos.y, v_in.pos.z, 1.f);
    //v_out.pos   = mul(v_out.pos, v_in.model);
    v_out.pos = mul(v_out.pos, test_buffer[inst_id].model);
    v_out.pos   = mul(v_out.pos, view_proj);

    v_out.color = v_in.color;

    return v_out;
}
