struct VERTEX_IN
{
    float3 pos   : POSITION;
    float4 color : COLOR;
};

struct VERTEX_OUT
{
    float4 pos   : SV_POSITION;
    float4 color : COLOR1;
};

VERTEX_OUT main(VERTEX_IN v_in)
{
    VERTEX_OUT v_out;

    v_out.pos   = float4(v_in.pos, 1.f);
    v_out.color = v_in.color;

    return v_out;
}
