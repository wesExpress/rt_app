struct vertex
{
    float2 position   : POSITION;
    float2 tex_coords : TEX_COORDS;
    float4 color      : COLOR;
};

struct fragment
{
    float4 position   : SV_Position;
    float2 tex_coords : TEX_COORDS1;
    float4 color      : COLOR1;
};

struct camera_data
{
    matrix projection;
};

struct resource_indices
{
    uint camera_data_index;
    uint texture_index;
    uint sampler_index;
};

ConstantBuffer<resource_indices> resources : register(b0);

fragment main(vertex v)
{
    ConstantBuffer<camera_data> c = ResourceDescriptorHeap[resources.camera_data_index];

    fragment f;

    f.position   = mul(c.projection, float4(v.position,0,1));
    f.tex_coords = v.tex_coords;
    f.color      = v.color;

    return f;
}
