struct VERTEX_IN
{
    float4 position_u : POSITION;
    float4 normal_v   : NORMAL;
    matrix model      : MODEL;
};

struct FRAGMENT_OUT 
{
    float4 pos        : SV_POSITION;
    float2 tex_coords : TEX_COORDS1;
};

struct scene_data
{
    matrix view_proj;
};

struct render_resources 
{
    uint scene_cb;
    uint sampler_index;
    uint diffuse_texture_index;
};

ConstantBuffer<render_resources> resources : register(b0);

FRAGMENT_OUT main(VERTEX_IN v_in, uint inst_id : SV_InstanceID)
{
    FRAGMENT_OUT frag_out;

    ConstantBuffer<scene_data> scene_buffer = ResourceDescriptorHeap[resources.scene_cb];

    frag_out.pos = mul(float4(v_in.position_u.xyz, 1), v_in.model);
    frag_out.pos = mul(frag_out.pos, scene_buffer.view_proj);

    frag_out.tex_coords = float2(v_in.position_u.w, v_in.normal_v.w);

    return frag_out;
}
