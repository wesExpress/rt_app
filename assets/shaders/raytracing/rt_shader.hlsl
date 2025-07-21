struct ray_payload
{
    float4 color;
};

struct camera_data
{
    matrix inv_view, inv_proj;
    float4 origin;
};

struct scene_data
{
    float4 sky_color;
};

struct vertex
{
    float4 position_u;
    float4 normal_v;
};

struct instance
{
    matrix model;
};

struct material
{
    uint vb_index;
    uint ib_index;
    uint is_indexed;
    uint diffuse_texture_index;
    uint sampler_index;
};

struct render_resources
{
    uint acceleration_structure;
    uint image;
    uint camera_data;
    uint scene_data;
    uint material_buffer;
};

ConstantBuffer<render_resources> resources : register(b0);

inline float3 get_direction(uint2 index, uint2 screen_dimensions, float3 origin, matrix inv_view, matrix inv_proj)
{
    const float  aspect_ratio = float(screen_dimensions.x) / float(screen_dimensions.y);

    // get screen position
    const float2 ndc  = (float2(index) + 0.5f) / float2(screen_dimensions);
    float2 screen_pos = ndc * 2.f - 1.f;
    screen_pos.y *= -1.f;

    // world position
    float3 world_pos = mul(float4(screen_pos, -1,1), inv_proj).xyz;
    world_pos        = mul(float4(world_pos, 1),     inv_view).xyz;

    // direction
    return normalize(world_pos - origin);
}

[shader("raygeneration")]
void ray_generation()
{
    RaytracingAccelerationStructure scene = ResourceDescriptorHeap[resources.acceleration_structure];
    RWTexture2D<float4> image             = ResourceDescriptorHeap[resources.image];
    ConstantBuffer<camera_data> camera_cb = ResourceDescriptorHeap[resources.camera_data];
    ConstantBuffer<scene_data> scene_cb   = ResourceDescriptorHeap[resources.scene_data];

    ray_payload p;
    p.color = float4(1,0,1,1);
    
    const uint2 launch_index = DispatchRaysIndex().xy;
    const uint2 screen_dimensions = DispatchRaysDimensions().xy;
    const float3 origin      = camera_cb.origin.xyz;

    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = get_direction(launch_index, screen_dimensions, origin, camera_cb.inv_view, camera_cb.inv_proj);
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

inline float3 interpolate_float3(float3 input[3], float3 barycentrics)
{
    return input[0] * barycentrics.x +
           input[1] * barycentrics.y +
           input[2] * barycentrics.z;
}

inline float2 interpolate_float2(float2 input[3], float3 barycentrics)
{
    return input[0] * barycentrics.x +
           input[1] * barycentrics.y +
           input[2] * barycentrics.z;
}

inline void get_vertices_indexed(material m, inout vertex vertices[3])
{
    const uint tri_index = PrimitiveIndex() * 3;

    StructuredBuffer<uint>   index_buffer  = ResourceDescriptorHeap[m.ib_index];
    StructuredBuffer<vertex> vertex_buffer = ResourceDescriptorHeap[m.vb_index];
    
    const uint indices[3] = {
        index_buffer[tri_index + 0],
        index_buffer[tri_index + 1],
        index_buffer[tri_index + 2]
    };

    vertices[0] = vertex_buffer[indices[0]];
    vertices[1] = vertex_buffer[indices[1]];
    vertices[2] = vertex_buffer[indices[2]];
}

inline void get_vertices(material m, inout vertex vertices[3])
{
    const uint tri_index = PrimitiveIndex() * 3;

    StructuredBuffer<vertex> vertex_buffer = ResourceDescriptorHeap[m.vb_index];
    
    vertices[0] = vertex_buffer[tri_index + 0];
    vertices[1] = vertex_buffer[tri_index + 1];
    vertices[2] = vertex_buffer[tri_index + 2];
}

[shader("closesthit")]
void closest_hit(inout ray_payload p, BuiltInTriangleIntersectionAttributes attrs)
{
    const float3 light = { 0,0,0 };

    const uint inst_index = InstanceIndex();

    StructuredBuffer<material> material_buffer = ResourceDescriptorHeap[resources.material_buffer];
    material m = material_buffer[inst_index];

    Texture2D diffuse_texture = ResourceDescriptorHeap[m.diffuse_texture_index];
    SamplerState sampler = SamplerDescriptorHeap[m.sampler_index];

    vertex vertices[3];

    if(m.is_indexed==0) get_vertices(m, vertices);
    else                get_vertices_indexed(m, vertices);

    float3 barycentrics = get_barycentrics(attrs);

    const float3 positions[3] = {
        vertices[0].position_u.xyz,
        vertices[1].position_u.xyz,
        vertices[2].position_u.xyz
    };

    const float2 tex_coords[3] = {
        { vertices[0].position_u.w, vertices[0].normal_v.w },
        { vertices[1].position_u.w, vertices[1].normal_v.w },
        { vertices[2].position_u.w, vertices[2].normal_v.w }
    };

    float2 uv = interpolate_float2(tex_coords, barycentrics);

#if 0
    float3 position = interpolate_float3(positions, barycentrics);
    position = mul(float4(position, 1), ObjectToWorld4x3()).xyz;

    float3 a = mul(float4(positions[0], 1), ObjectToWorld4x3()).xyz;
    float3 b = mul(float4(positions[1], 1), ObjectToWorld4x3()).xyz;
    float3 c = mul(float4(positions[2], 1), ObjectToWorld4x3()).xyz;

    float3 ab = b - a;
    float3 ac = c - a;

    float3 normal = normalize(cross(ab, ac));
#endif

    p.color = diffuse_texture.SampleGrad(sampler, uv, 0,0);
}

