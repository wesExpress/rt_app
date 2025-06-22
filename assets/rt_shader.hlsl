struct ray_payload
{
    float4 color;
};

struct scene_data
{
    matrix inv_view, inv_proj, inv_vp;
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
};

ConstantBuffer<render_resources> resources : register(b0);

inline float3 get_ray_direction(const float3 origin, const matrix inv_proj, const matrix inv_view)
{
    const uint2 launch_index      = DispatchRaysIndex().xy;
    const uint2 screen_dimensions = DispatchRaysDimensions().xy;
    const float  aspect_ratio     = float(screen_dimensions.x) / float(screen_dimensions.y);

    // get screen position
    const float2 ndc  = (float2(launch_index) + 0.5f) / float2(screen_dimensions);
    float2 screen_pos = ndc * 2.f - 1.f;
    screen_pos.y *= -1.f;

    // world position
    float3 world_pos = mul(float4(screen_pos, -1,1), inv_proj).xyz;
    world_pos        = mul(float4(world_pos, 1), inv_view).xyz;

    // direction
    return normalize(world_pos - origin);
}

[shader("raygeneration")]
void ray_generation()
{
    RaytracingAccelerationStructure scene = ResourceDescriptorHeap[resources.acceleration_structure];
    RWTexture2D<float4> image             = ResourceDescriptorHeap[resources.image];
    ConstantBuffer<scene_data> scene_cb   = ResourceDescriptorHeap[resources.scene_data];

    ray_payload p;
    p.color.rgb = float3(1,0,1);
    
    const uint2 launch_index = DispatchRaysIndex().xy;
    const float3 origin      = scene_cb.origin.xyz;

    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = get_ray_direction(origin, scene_cb.inv_proj, scene_cb.inv_view);
    ray.TMin      = 0.001f;
    ray.TMax      = 1000.f;

    TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0,1,0, ray, p);

    image[launch_index] = float4(p.color.rgb, 1.f);
}

[shader("miss")]
void miss(inout ray_payload p)
{
    ConstantBuffer<scene_data> scene_cb = ResourceDescriptorHeap[resources.scene_data];

    p.color.rgb = scene_cb.sky_color.rgb;
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

