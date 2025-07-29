#define SAMPLES_PER_PIXEL 1

#include "../lighting.h"

struct ray_payload
{
    float4 color;
    uint   bounce_count;
};

struct shadow_ray_payload
{
    float attenuation;
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
    float4 tangent;
    float4 color;
};

struct instance
{
    matrix model;
};

struct material
{
    uint diffuse_texture_index;
    uint metallic_texture_index;
    uint normal_map_index;
    uint specular_map_index;
    uint occlusion_map_index;
    uint emissive_map_index;

    uint diffuse_sampler_index;
    uint metallic_sampler_index;
    uint normal_sampler_index;
    uint specular_sampler_index;
    uint occlusion_sampler_index;
    uint emissive_sampler_index;

    uint padding[2];
};

struct mesh_data
{
    uint vb_index, ib_index;
    uint padding[2];
};

struct node_data
{
    uint   mesh_index, material_index;
    uint   padding[2];
    matrix model;
};

struct light_source
{
    float3 position;
    float3 color;
    float3 ambient;
    float  strength;
};

struct render_resources
{
    uint acceleration_structure;
    uint image;
    uint random_image;
    uint camera_data;
    uint scene_data;
    uint node_buffer_index;
    uint material_buffer_index;
    uint mesh_buffer_index;
    uint light_buffer_index;
};

ConstantBuffer<render_resources> resources : register(b0);

inline float random_float(inout uint seed)
{
    seed = (1664525u * seed + 1013904223u);
	return float(seed & 0x00FFFFFF) / float(0x01000000);
}

inline float3 get_direction(uint2 index, uint2 screen_dimensions, float3 origin, matrix inv_view, matrix inv_proj, inout uint seed)
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
    RWTexture2D<uint> random_image        = ResourceDescriptorHeap[resources.random_image];
    ConstantBuffer<camera_data> camera_cb = ResourceDescriptorHeap[resources.camera_data];
    ConstantBuffer<scene_data> scene_cb   = ResourceDescriptorHeap[resources.scene_data];

    const uint2 launch_index      = DispatchRaysIndex().xy;
    const uint2 screen_dimensions = DispatchRaysDimensions().xy;
    const float3 origin           = camera_cb.origin.xyz;

    uint seed = random_image[launch_index];

    float4 color = float4(0,0,0,1);
    for(uint i=0; i<SAMPLES_PER_PIXEL; i++)
    {
        ray_payload p;
        p.color = float4(1,0,1,1);

        RayDesc ray;
        ray.Origin    = origin;
        ray.Direction = get_direction(launch_index, screen_dimensions, origin, camera_cb.inv_view, camera_cb.inv_proj, seed);
        ray.TMin      = 0.001f;
        ray.TMax      = 1000.f;

        TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0,0,0, ray, p);

        color += p.color;
    }   

    color.rgb /= (float)SAMPLES_PER_PIXEL;

    image[launch_index] = color;
    random_image[launch_index] = seed;
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

inline void get_vertices_indexed(mesh_data m, inout vertex vertices[3])
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

[shader("closesthit")]
void closest_hit(inout ray_payload p, BuiltInTriangleIntersectionAttributes attrs)
{
    const uint inst_index = InstanceIndex();

    RaytracingAccelerationStructure scene = ResourceDescriptorHeap[resources.acceleration_structure];

    StructuredBuffer<node_data> node_buffer    = ResourceDescriptorHeap[resources.node_buffer_index];
    StructuredBuffer<mesh_data> mesh_buffer    = ResourceDescriptorHeap[resources.mesh_buffer_index];
    StructuredBuffer<material> material_buffer = ResourceDescriptorHeap[resources.material_buffer_index];
    StructuredBuffer<light_source> lights      = ResourceDescriptorHeap[resources.light_buffer_index];

    node_data node = node_buffer[inst_index];
    mesh_data mesh = mesh_buffer[node.mesh_index];
    material  mat  = material_buffer[node.material_index];

    Texture2D    diffuse_texture = ResourceDescriptorHeap[mat.diffuse_texture_index];
    Texture2D    metallic_map    = ResourceDescriptorHeap[mat.metallic_texture_index];
    Texture2D    normal_map      = ResourceDescriptorHeap[mat.normal_map_index];
    Texture2D    occlusion_map   = ResourceDescriptorHeap[mat.occlusion_map_index];
    Texture2D    emissive_map    = ResourceDescriptorHeap[mat.emissive_map_index];

    SamplerState diffuse_sampler   = SamplerDescriptorHeap[mat.diffuse_sampler_index];
    SamplerState metallic_sampler  = SamplerDescriptorHeap[mat.metallic_sampler_index];
    SamplerState normal_sampler    = SamplerDescriptorHeap[mat.normal_sampler_index];
    SamplerState occlusion_sampler = SamplerDescriptorHeap[mat.occlusion_sampler_index];
    SamplerState emissive_sampler  = SamplerDescriptorHeap[mat.emissive_sampler_index];

    // vertex interpolation 
    vertex vertices[3];

    get_vertices_indexed(mesh, vertices);

    float3 barycentrics = get_barycentrics(attrs);

    const float3 positions[3] = {
        vertices[0].position_u.xyz,
        vertices[1].position_u.xyz,
        vertices[2].position_u.xyz
    };

    const float3 normals[3] = {
        vertices[0].normal_v.xyz,
        vertices[1].normal_v.xyz,
        vertices[2].normal_v.xyz
    };

    const float3 tangents[3] = {
        vertices[0].tangent.xyz,
        vertices[1].tangent.xyz,
        vertices[2].tangent.xyz
    };

    const float2 tex_coords[3] = {
        { vertices[0].position_u.w, vertices[0].normal_v.w },
        { vertices[1].position_u.w, vertices[1].normal_v.w },
        { vertices[2].position_u.w, vertices[2].normal_v.w }
    };

    float3 position = interpolate_float3(positions, barycentrics);
    float3 normal   = interpolate_float3(normals, barycentrics);
    float3 tangent  = interpolate_float3(tangents, barycentrics);
    float2 uv       = interpolate_float2(tex_coords, barycentrics);

    position = mul(float4(position, 1), ObjectToWorld4x3()).xyz;

    float3 T = normalize(mul(float4(tangent, 0), ObjectToWorld4x3()).xyz);
    float3 N = normalize(mul(float4(normal, 0), ObjectToWorld4x3()).xyz);
    float3 B = cross(N,T);

    float3x3 TBN = float3x3(T,B,N);

    normal = normal_map.SampleGrad(normal_sampler, uv, 0,0).rgb;
    normal = normalize(normal * 2 - 1);
    normal = normalize(mul(normal, TBN));

    // pbr values
    float3 diffuse_color      = diffuse_texture.SampleGrad(diffuse_sampler, uv, 0,0).rgb;
    float3 metallic_roughness = metallic_map.SampleGrad(metallic_sampler, uv, 0,0).rgb;
    float3 occlusion          = occlusion_map.SampleGrad(occlusion_sampler, uv, 0,0).rgb;
    float3 emission           = emissive_map.SampleGrad(emissive_sampler, uv, 0,0).rgb;

    float metallic  = metallic_roughness.b;
    float roughness = metallic_roughness.g;

    // lighting
    light_source light = lights[0];

    float3 color = calculate_lighting(position, normal, light.position, light.color, light.ambient, diffuse_color, WorldRayOrigin(), roughness, metallic);

    // shadows
    shadow_ray_payload shadow_p;
    shadow_p.attenuation = 0;

    float3 light_dir = light.position - position;

    RayDesc shadow_ray;
    shadow_ray.Origin    = position; 
    shadow_ray.Direction = normalize(light_dir);
    shadow_ray.TMin      = 0.001f;
    shadow_ray.TMax      = length(light_dir);

    TraceRay(scene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0,0,1, shadow_ray, shadow_p);

    p.color.rgb = light.ambient * occlusion + color * shadow_p.attenuation + emission;
}

/*************
* SHADOW RAY *
**************/
[shader("miss")]
void shadow_miss(inout shadow_ray_payload p)
{
    p.attenuation = 1.f;
}
