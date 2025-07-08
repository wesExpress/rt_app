struct PIXEL_IN 
{
    float4 pos       : SV_POSITION;
    float3 normal    : NORMAL1;
    float4 color     : COLOR1;
    float4 world_pos : POSITION1;
};

struct PIXEL_OUT
{
    float4 color : SV_TARGET;
};

PIXEL_OUT main(PIXEL_IN p_in)
{
    PIXEL_OUT p_out;

    p_out.color = p_in.color;

    return p_out;
}
