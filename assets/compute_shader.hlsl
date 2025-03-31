struct transform
{
    float4 position;
    float4 scale;
    float4 orientation;
};

struct instance
{
    float4x4 model;
    float4x4 normal;
};

#define IDENTITY float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)

RWStructuredBuffer<transform> transforms : register(u0);
RWStructuredBuffer<instance>  instances  : register(u1);

float4x4 translate(float4x4 matrix, float3 position)
{
    matrix[0][3] = position.x;
    matrix[1][3] = position.y;
    matrix[2][3] = position.z;

    return matrix;
}

[numthreads(64,1,1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_DispatchThreadID, uint3 group_thread_id : SV_GroupThreadID, uint group_index : SV_GroupIndex)
{
    const int index = thread_id.x;

    transform t = transforms[index];

    matrix model = IDENTITY;
    
    model = translate(model, t.position);

    instances[index].model = transpose(model);
}
