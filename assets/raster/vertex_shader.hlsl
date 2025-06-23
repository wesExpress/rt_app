struct VERTEX_IN
{
    float4 pos      : POSITION;
    float4 normal   : NORMAL;
    float4 color    : COLOR;
};

struct VERTEX_OUT
{
    float4 pos    : SV_POSITION;
    float3 normal : NORMAL1;
    float4 color  : COLOR1;
};

struct instance
{
    float4x4 model;
    float4x4 normal;
};

struct scene_data
{
    matrix view_proj;
};

struct render_resources 
{
    uint scene_cb;
    uint inst_b;
};

ConstantBuffer<render_resources> resources : register(b0);

VERTEX_OUT main(VERTEX_IN v_in, uint inst_id : SV_InstanceID)
{
    VERTEX_OUT v_out;

    ConstantBuffer<scene_data> scene_buffer    = ResourceDescriptorHeap[resources.scene_cb];
    StructuredBuffer<instance> instance_buffer = ResourceDescriptorHeap[resources.inst_b];

    v_out.pos = float4(v_in.pos.x, v_in.pos.y, v_in.pos.z, 1.f);
    v_out.pos = mul(v_out.pos, instance_buffer[inst_id].model);
    v_out.pos = mul(v_out.pos, scene_buffer.view_proj);

    v_out.color = v_in.color;

    return v_out;
}
