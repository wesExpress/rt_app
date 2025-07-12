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

struct instance
{
    matrix model;
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
    Texture2D<float4> ray_image           = ResourceDescriptorHeap[resources.ray_image];

    ray_payload p;
    p.color = float4(1,0,1,1);
    
    const uint2 launch_index = DispatchRaysIndex().xy;
    const float3 origin      = scene_cb.origin.xyz;

    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = ray_image[launch_index].xyz;
    ray.TMin      = 0.001f;
    ray.TMax      = 1000.f;

    TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0,1,0, ray, p);

    image[launch_index] = float4(p.color.rgb, 1.f);
}

[shader("miss")]
void miss(inout ray_payload p)
{
    ConstantBuffer<scene_data> scene_cb   = ResourceDescriptorHeap[resources.scene_data];

    p.color = scene_cb.sky_color;
}

inline float3 get_barycentrics(BuiltInTriangleIntersectionAttributes attrs)
{
    return float3(
        1 - attrs.barycentrics.x - attrs.barycentrics.y,
        attrs.barycentrics.x,
        attrs.barycentrics.y
    );
}

inline float3 interpolate_value(float3 input[3], float3 barycentrics)
{
    return input[0] * barycentrics.x +
           input[1] * barycentrics.y +
           input[2] * barycentrics.z;
}

[shader("closesthit")]
void closest_hit(inout ray_payload p, BuiltInTriangleIntersectionAttributes attrs)
{
    const float3 light = { 0,0,0 };

    const uint tri_index  = PrimitiveIndex() * 3;
    const uint inst_index = InstanceIndex();

    StructuredBuffer<material> material_buffer = ResourceDescriptorHeap[resources.material_buffer];
    StructuredBuffer<vertex>   vertex_buffer   = ResourceDescriptorHeap[material_buffer[inst_index].vb_index];
    StructuredBuffer<uint>     index_buffer    = ResourceDescriptorHeap[material_buffer[inst_index].ib_index];
    
    const uint3 indices = {
        index_buffer[tri_index + 0],
        index_buffer[tri_index + 1],
        index_buffer[tri_index + 2]
    };

    const vertex vertices[3] = {
        vertex_buffer[indices[0]],
        vertex_buffer[indices[1]],
        vertex_buffer[indices[2]]
    };

    float3 positions[3] = {
        vertices[0].position.xyz,
        vertices[1].position.xyz,
        vertices[2].position.xyz
    };

    float3 normals[3] = {
        vertices[0].normal.xyz,
        vertices[1].normal.xyz,
        vertices[2].normal.xyz
    };

    const float3 colors[3] = {
        vertices[0].color.rgb,
        vertices[1].color.rgb,
        vertices[2].color.rgb
    };

    float3 barycentrics = get_barycentrics(attrs);

    float3 position = interpolate_value(positions, barycentrics);
    float3 color    = interpolate_value(colors, barycentrics);

    position = mul(float4(position, 1), ObjectToWorld4x3()).xyz;

    float3 a = mul(float4(positions[0], 1), ObjectToWorld4x3()).xyz;
    float3 b = mul(float4(positions[1], 1), ObjectToWorld4x3()).xyz;
    float3 c = mul(float4(positions[2], 1), ObjectToWorld4x3()).xyz;

    float3 ab = b - a;
    float3 ac = c - a;

    float3 normal = normalize(cross(ab, ac));

    p.color.rgb = color * dot(light - position, normal);
}

