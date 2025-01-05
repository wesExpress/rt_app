struct VERTEX_IN
{
    float4 pos : SV_POSITION;
};

struct PIXEL_OUT
{
    float4 color : SV_TARGET;
};

PIXEL_OUT main(VERTEX_IN v_in)
{
    PIXEL_OUT p_out;

    p_out.color = float4(1.f,0,0,1.f);

    return p_out;
}
