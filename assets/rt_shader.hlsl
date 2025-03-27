struct ray_payload
{
    float3 color;
    bool   missed;
};

RaytracingAccelerationStructure scene : register(t0);

RWTexture2D<float4> image : register(u0);

cbuffer ConstantBuffer : register(b0)
{
    matrix view_proj;
};

[shader("raygeneration")]
void ray_generation()
{
    uint2 launch_index      = DispatchRaysIndex().xy;
    uint2 screen_dimensions = DispatchRaysDimensions().xy;

    ray_payload p;
    p.color = float3(0,1,1);

    RayDesc ray;
    ray.Origin    = float3(0,0,0);
    ray.Direction = float3(1,0,0);
    ray.TMin      = 0.01f;
    ray.TMax      = 1000.f;

    TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0,1,0, ray, p);

    image[launch_index] = float4(p.color, 1.f);
}

[shader("closesthit")]
void closest_hit(inout ray_payload p, BuiltInTriangleIntersectionAttributes attrs)
{
    p.color = float3(1,1,1);
}

[shader("miss")]
void miss(inout ray_payload p)
{
    p.color = float3(0,0,0);
}
