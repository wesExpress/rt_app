struct transform
{
    float4 position;
    float4 scale;
    float4 orientation;
};

struct physics
{
    float3 w;
};

struct instance
{
    matrix model;
};

struct rt_instance
{
    float4 transform[3];

    uint     id : 24;
    uint     mask : 8;
    uint     sbt_offset : 24;
    uint     flags : 8;
    uint64_t blas;
};

#define IDENTITY float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)

struct render_resources
{
    uint transform_buffer;
    uint physics_buffer;
    uint instance_buffer;
    uint rt_instance_buffer;
    uint rt_blas_buffer;
};

ConstantBuffer<render_resources> resources : register(b0);

matrix matrix_translate(float4 position)
{
    matrix translation = IDENTITY;

    translation[0][3] = position.x;
    translation[1][3] = position.y;
    translation[2][3] = position.z;

    return translation;
}

matrix matrix_scale( float4 scale)
{
    matrix scaling = IDENTITY;

    scaling[0][0] = scale.x;
    scaling[1][1] = scale.y;
    scaling[2][2] = scale.z;

    return scaling;
}

matrix matrix_rotate(float4 orientation)
{
    matrix rotation = IDENTITY;

    const float x = orientation[0];
    const float y = orientation[1];
    const float z = orientation[2];
    const float w = orientation[3];

    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;
    
    const float xy = x * y;
    const float xz = x * z;
    const float xw = x * w;
    const float yz = y * z;
    const float yw = y * w;
    const float zw = z * w;
    
    rotation[0][0] = 1 - 2 * (yy + zz);
    rotation[0][1] = 2 * (xy - zw);
    rotation[0][2] = 2 * (xz + yw);
    
    rotation[1][0] = 2 * (xy + zw);
    rotation[1][1] = 1 - 2 * (xx + zz);
    rotation[1][2] = 2 * (yz - xw);
    
    rotation[2][0] = 2 * (xz - yw);
    rotation[2][1] = 2 * (yz + xw);
    rotation[2][2] = 1 - 2 * (xx + yy);

    return rotation;
}

float4 update_orientation(float4 orientation, float3 rotation)
{
    float4 new_orientation;

    new_orientation.x = orientation.x + rotation.x * orientation.w + rotation.y * orientation.z - rotation.z * orientation.y;
    new_orientation.y = orientation.y - rotation.x * orientation.z + rotation.y * orientation.w + rotation.z * orientation.x;
    new_orientation.z = orientation.z + rotation.x * orientation.y - rotation.y * orientation.x + rotation.z * orientation.w;
    new_orientation.w = orientation.w - rotation.x * orientation.x - rotation.y * orientation.y - rotation.z * orientation.z;

    return normalize(new_orientation);
}

[numthreads(8,1,1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_DispatchThreadID, uint3 group_thread_id : SV_GroupThreadID, uint group_index : SV_GroupIndex)
{
    const int index = thread_id.x;

    RWStructuredBuffer<transform>   transforms     = ResourceDescriptorHeap[resources.transform_buffer];
    RWStructuredBuffer<physics>     phys           = ResourceDescriptorHeap[resources.physics_buffer];
    RWStructuredBuffer<instance>    instances      = ResourceDescriptorHeap[resources.instance_buffer]; 
    RWStructuredBuffer<rt_instance> rt_instances   = ResourceDescriptorHeap[resources.rt_instance_buffer];
    RWStructuredBuffer<uint64_t>    rt_blas_buffer = ResourceDescriptorHeap[resources.rt_blas_buffer];

    transform t = transforms[index];
    physics   p = phys[index];

    matrix model = IDENTITY;

    matrix scaling     = matrix_scale(t.scale);
    matrix rotation    = matrix_rotate(t.orientation);
    matrix translation = matrix_translate(t.position);

    model = mul(scaling,     model);
    model = mul(rotation,    model);
    model = mul(translation, model);

    instances[index].model = transpose(model);

    rt_instances[index].transform[0] = model[0];
    rt_instances[index].transform[1] = model[1];
    rt_instances[index].transform[2] = model[2];

    rt_instances[index].id = index;

    uint count = 2;
    const float half_dt = 0.0016 * 0.5f;
    float3 quat_rotate = p.w * half_dt;
    while(count > 0)
    {
        transforms[index].orientation = update_orientation(t.orientation, quat_rotate);
        count--;
    }
}
