struct camera_data
{
    matrix inv_view;
    matrix inv_proj;
    float4 position;
};

struct image_data
{
    uint width;
    uint height;
};

struct resource_data
{
    uint camera_data;
    uint image_data;
    uint ray_image;
};

ConstantBuffer<resource_data> resources : register(b0);

[numthreads(8,8,1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_DispatchThreadID, uint3 group_thread_id : SV_GroupThreadID, uint group_index : SV_GroupIndex)
{
    ConstantBuffer<camera_data> camera_buffer = ResourceDescriptorHeap[resources.camera_data];
    ConstantBuffer<image_data>  image_buffer  = ResourceDescriptorHeap[resources.image_data];
    RWTexture2D<float4>         ray_image     = ResourceDescriptorHeap[resources.ray_image];

    const uint2 index = thread_id.xy;
    const uint2 screen_dimensions = uint2(image_buffer.width, image_buffer.height); 
    const float  aspect_ratio     = float(screen_dimensions.x) / float(screen_dimensions.y);

    // get screen position
    const float2 ndc  = (float2(index) + 0.5f) / float2(screen_dimensions);
    float2 screen_pos = ndc * 2.f - 1.f;
    screen_pos.y *= -1.f;

    // world position
    float3 world_pos = mul(float4(screen_pos, -1,1), camera_buffer.inv_proj).xyz;
    world_pos        = mul(float4(world_pos, 1),     camera_buffer.inv_view).xyz;

    // direction
    ray_image[index] = float4(normalize(world_pos - camera_buffer.position.xyz), 1);
}
