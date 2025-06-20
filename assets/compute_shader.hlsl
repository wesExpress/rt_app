struct transform
{
    float4 position;
    float4 scale;
    float4 orientation;
};

struct instance
{
    matrix model;
    matrix normal;
};

struct scene_data
{
    double delta;
};

#define IDENTITY float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)

struct render_resources
{
    uint transform_buffer;
    uint instance_buffer;
    uint scene_data;
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

[numthreads(64,1,1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_DispatchThreadID, uint3 group_thread_id : SV_GroupThreadID, uint group_index : SV_GroupIndex)
{
    const int index = thread_id.x;

    RWStructuredBuffer<transform> transforms = ResourceDescriptorHeap[resources.transform_buffer];
    RWStructuredBuffer<instance>  instances  = ResourceDescriptorHeap[resources.instance_buffer]; 
    ConstantBuffer<scene_data>    scene_cb   = ResourceDescriptorHeap[resources.scene_data];

    transform t = transforms[index];

    matrix model = IDENTITY;

    matrix translation = matrix_translate(t.position);
    matrix rotation    = matrix_rotate(t.orientation);
    matrix scaling     = matrix_scale(t.scale);

    model = mul(translation, mul(rotation, scaling));

    instances[index].model = transpose(model);

    float3 quat_rotate = float3(0,scene_cb.delta,0);
    transforms[index].orientation = update_orientation(t.orientation, quat_rotate);
}
