struct VERTEX_IN
{
    float4 position_u : POSITION;
    float4 normal_v   : NORMAL;
    float4 tangent    : TANGENT;
    float4 color      : COLOR;
};

struct FRAGMENT_OUT 
{
    float4 fragment_pos : SV_POSITION;
    float4 world_pos    : POSITION1;
    float2 tex_coords   : TEX_COORDS1;
    float3x3 tbn        : TBN;
    uint instance_index : INST;
};

struct scene_data
{
    matrix view_proj;
};

struct node_data
{
    uint   mesh_index, material_index;
    uint   padding[2];
    matrix model;
};

struct render_resources 
{
    uint scene_cb;
    uint material_buffer_index;
    uint mesh_buffer_index;
    uint node_buffer_index;
    uint light_buffer_index;
};

ConstantBuffer<render_resources> resources : register(b0);

FRAGMENT_OUT main(VERTEX_IN v_in, uint inst_id : SV_InstanceID)
{
    FRAGMENT_OUT frag_out;

    ConstantBuffer<scene_data> scene_buffer = ResourceDescriptorHeap[resources.scene_cb];
    StructuredBuffer<node_data> nodes       = ResourceDescriptorHeap[resources.node_buffer_index];

    node_data node = nodes[inst_id];

    frag_out.world_pos    = mul(float4(v_in.position_u.xyz, 1), node.model);
    frag_out.fragment_pos = mul(frag_out.world_pos, scene_buffer.view_proj);

    frag_out.tex_coords = float2(v_in.position_u.w, v_in.normal_v.w);

    frag_out.instance_index = inst_id;

    float3 N = normalize(mul(float4(v_in.normal_v.xyz, 0), node.model).xyz);
    float3 T = normalize(mul(v_in.tangent, node.model).xyz);
    float3 B = normalize(cross(N,T));

    frag_out.tbn = float3x3(T,B,N);

    return frag_out;
}
