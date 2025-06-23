struct ray_payload
{
    float4 color;
};

struct scene_data
{
	float4 origin;
    float4 sky_color;
};

struct vertex
{
    float4 position;
    float4 normal;
    float4 color;
};

struct material
{
    uint vb_index;
    uint ib_index;
};

struct render_resources
{
    uint acceleration_structure;
    uint image;
    uint scene_data;
    uint material_buffer;
    uint ray_image;
};

ConstantBuffer<render_resources> resources : register(b0);

[shader("raygeneration")]
void ray_generation()
{
    RaytracingAccelerationStructure scene = ResourceDescriptorHeap[resources.acceleration_structure];
    RWTexture2D<float4> image             = ResourceDescriptorHeap[resources.image];
    ConstantBuffer<scene_data> scene_cb   = ResourceDescriptorHeap[resources.scene_data];
    RWTexture2D<float4> ray_image         = ResourceDescriptorHeap[resources.ray_image];

    ray_payload p;
    p.color = float4(1,0,1,1);
    
    const uint2 launch_index = DispatchRaysIndex().xy;
    const float3 origin      = scene_cb.origin.xyz;

    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = ray_image[launch_index].xyz;
    ray.TMin      = 0.001f;
    ray.TMax      = 100.f;

    TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0,1,0, ray, p);

    image[launch_index] = float4(p.color.rgb, 1.f);
}

[shader("miss")]
void miss(inout ray_payload p)
{
    ConstantBuffer<scene_data> scene_cb   = ResourceDescriptorHeap[resources.scene_data];

    p.color = scene_cb.sky_color;
}

[shader("closesthit")]
void closest_hit(inout ray_payload p, BuiltInTriangleIntersectionAttributes attrs)
{
    const uint tri_index  = PrimitiveIndex() * 3;
    const uint inst_index = InstanceIndex();

    StructuredBuffer<material> material_buffer = ResourceDescriptorHeap[resources.material_buffer];
    StructuredBuffer<vertex>   vertex_buffer   = ResourceDescriptorHeap[material_buffer[inst_index].vb_index];
    StructuredBuffer<uint>     index_buffer    = ResourceDescriptorHeap[material_buffer[inst_index].ib_index];
    
    float3 barycentrics = float3(
        1 - attrs.barycentrics.x - attrs.barycentrics.y,
        attrs.barycentrics.x,
        attrs.barycentrics.y
    );

    uint3 indices = {
        index_buffer[tri_index + 0],
        index_buffer[tri_index + 1],
        index_buffer[tri_index + 2]
    };

    float3 color = 
        vertex_buffer[indices[0]].color.rgb * barycentrics.x +
        vertex_buffer[indices[1]].color.rgb * barycentrics.y +
        vertex_buffer[indices[2]].color.rgb * barycentrics.z;  

    p.color.rgb = color;
}

