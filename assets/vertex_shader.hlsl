struct VERTEX_IN
{
    float3 pos   : POSITION;
    float4 color : COLOR;
    matrix model : MODEL;
};

struct VERTEX_OUT
{
    float4 pos   : SV_POSITION;
    float4 color : COLOR1;
};

cbuffer ConstantBuffer : register(b0)
{
    matrix view_proj;
};

VERTEX_OUT main(VERTEX_IN v_in)
{
    VERTEX_OUT v_out;

    v_out.pos   = float4(v_in.pos.x, v_in.pos.y, v_in.pos.z, 1.f);
    v_out.pos   = mul(v_out.pos, v_in.model);
    v_out.pos   = mul(v_out.pos, view_proj);

    v_out.color = v_in.color;

    return v_out;
}
