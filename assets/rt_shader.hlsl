struct ray_payload
{
    float4 color;
};

struct scene_data
{
    matrix inv_view, inv_proj;
	float4 origin;
};

struct render_resources
{
    uint acceleration_structure;
    uint image;
    uint scene_data;
};

ConstantBuffer<render_resources> resources : register(b0);

[shader("raygeneration")]
void ray_generation()
{
    RaytracingAccelerationStructure scene = ResourceDescriptorHeap[resources.acceleration_structure];
    RWTexture2D<float4> image             = ResourceDescriptorHeap[resources.image];
    ConstantBuffer<scene_data> scene_cb   = ResourceDescriptorHeap[resources.scene_data];

    uint2 launch_index      = DispatchRaysIndex().xy;
    uint2 screen_dimensions = DispatchRaysDimensions().xy;

    float2 coords = float2(launch_index) / float2(screen_dimensions);
    coords = coords * 2.f - 1.f;
    coords.y *= -1.f;

    float4 target = mul(scene_cb.inv_proj, float4(coords, 0.f,1.f));
    target.xyz /= target.w;
    float3 direction = normalize(mul(scene_cb.inv_view, float4(target.xyz, 0)).xyz);

    ray_payload p;
    p.color.rgb = float3(1,0,1);

    RayDesc ray;
    ray.Origin    = scene_cb.origin.xyz;
    ray.Direction = direction;
    ray.TMin      = 0.001f;
    ray.TMax      = 1000.f;

    TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0,1,0, ray, p);

    image[launch_index] = float4(p.color.rgb, 1.f);
}

[shader("miss")]
void miss(inout ray_payload p)
{
    p.color.rgb = float3(0,0,0);
}

[shader("closesthit")]
void closest_hit(inout ray_payload p, BuiltInTriangleIntersectionAttributes attrs)
{
    p.color.rgb = float3(1,1,1);
}
