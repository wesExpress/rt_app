struct vertex_in
{
    float2 position   : POSITION;
    float2 tex_coords : TEX_COORDS;
};

struct vertex_out
{
    float4 position : SV_Position;
    float2 tex_coords : TEX_COORDS1;
};

vertex_out main(vertex_in v_in) 
{
    vertex_out v_out;

    v_out.position   = float4(v_in.position, 0, 1);
    v_out.tex_coords = v_in.tex_coords;

    return v_out;
}
