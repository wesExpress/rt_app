struct VERTEX_IN
{
    float3 pos : POSITION;
};

struct VERTEX_OUT
{
    float4 pos : SV_POSITION;
};

VERTEX_OUT main(VERTEX_IN v_in)
{
    VERTEX_OUT v_out;

    v_out.pos = float4(v_in.pos, 1.f);

    return v_out;
}
