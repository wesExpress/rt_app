#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_ARB_shading_language_include : enable

#define float3 vec3
#include "..\lighting.h"
#include "..\shader_include.h"

struct payload
{
    vec4 color;
    uint bounce_count;
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
    uint material_index;
    uint padding;
};

struct vertex
{
    vec4 position_u;
    vec4 normal_v;
    vec4 tangent;
    vec4 color;
};

layout(push_constant) uniform render_resources
{
    uint acceleration_structure_index;
    uint image_index, random_image_index;
    uint camera_data_index;
    uint scene_data_index;
    uint material_buffer_index;
    uint mesh_buffer_index;
};

layout(set=0, binding=0) uniform accelerationStructureEXT acceleration_structures[RESOURCE_HEAP_SIZE];

layout(set=0, binding=0) buffer b0 { mesh_data data[]; } mesh_buffers[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) buffer b1 { material data[]; }  material_buffers[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) buffer b2 { vertex data[]; }    vertex_buffers[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) buffer b3 { uint data[]; }      index_buffers[RESOURCE_HEAP_SIZE];

layout(set=0, binding=0) uniform texture2D textures[RESOURCE_HEAP_SIZE];
layout(set=1, binding=0) uniform sampler samplers[SAMPLER_HEAP_SIZE];

layout(location=0) rayPayloadInEXT payload p;
hitAttributeEXT vec2 attribs;

vec3 get_barycentrics(vec2 attribs)
{
    return vec3(
        1 - attribs.x - attribs.y,
        attribs.x,
        attribs.y
    );
}

vec3 interpolate_vec3(vec3 values[3], vec3 barycentrics)
{
    return values[0] * barycentrics.x +
           values[1] * barycentrics.y +
           values[2] * barycentrics.z;
}

vec2 interpolate_vec2(vec2 values[3], vec3 barycentrics)
{
    return values[0] * barycentrics.x +
           values[1] * barycentrics.y +
           values[2] * barycentrics.z;
}

void get_vertices_indexed(mesh_data m, inout vertex vertices[3])
{
    uint tri_index = gl_PrimitiveID * 3;

    uint indices[3] = {
        index_buffers[m.ib_index].data[tri_index + 0],
        index_buffers[m.ib_index].data[tri_index + 1],
        index_buffers[m.ib_index].data[tri_index + 2]
    };

    vertices[0] = vertex_buffers[m.vb_index].data[indices[0]];
    vertices[1] = vertex_buffers[m.vb_index].data[indices[1]];
    vertices[2] = vertex_buffers[m.vb_index].data[indices[2]];
}

void main()
{
    const vec3 light = vec3(0);

    uint inst_index = gl_InstanceCustomIndexEXT;

    mesh_data mesh = mesh_buffers[mesh_buffer_index].data[inst_index];
    material  mat  = material_buffers[material_buffer_index].data[mesh.material_index];

    vertex vertices[3];

    get_vertices_indexed(mesh, vertices);

    vec3 positions[3] = {
        vertices[0].position_u.xyz,
        vertices[1].position_u.xyz,
        vertices[2].position_u.xyz
    };

    vec3 normals[3] = {
        vertices[0].normal_v.xyz,
        vertices[1].normal_v.xyz,
        vertices[2].normal_v.xyz
    };

    vec3 tangents[3] = {
        vertices[0].tangent.xyz,
        vertices[1].tangent.xyz,
        vertices[2].tangent.xyz,
    };

    const vec2 tex_coords[3] = {
        { vertices[0].position_u.w, vertices[0].normal_v.w },
        { vertices[1].position_u.w, vertices[1].normal_v.w },
        { vertices[2].position_u.w, vertices[2].normal_v.w }
    };

    vec3 barycentrics = get_barycentrics(attribs);

    vec3 position = interpolate_vec3(positions, barycentrics);
    vec3 normal   = interpolate_vec3(normals, barycentrics);
    vec3 tangent  = interpolate_vec3(tangents, barycentrics);
    vec2 uv = interpolate_vec2(tex_coords, barycentrics);

    position = (gl_ObjectToWorldEXT * vec4(position, 1)).xyz;

    vec3 T = normalize((gl_ObjectToWorldEXT * vec4(position, 0)).xyz);
    vec3 N = normalize((gl_ObjectToWorldEXT * vec4(normal, 0)).xyz);
    vec3 B = normalize(cross(N,T));

    mat3 TBN = mat3(T,B,N);

    vec3 diffuse_color      = texture(sampler2D(textures[mat.diffuse_texture_index],  samplers[mat.diffuse_sampler_index]),   uv).rgb; 
    vec3 metallic_roughness = texture(sampler2D(textures[mat.metallic_texture_index], samplers[mat.metallic_sampler_index]),  uv).rgb;
    vec3 normal_map_normal  = texture(sampler2D(textures[mat.normal_map_index],       samplers[mat.normal_sampler_index]),    uv).rgb;
    vec3 occlusion          = texture(sampler2D(textures[mat.occlusion_map_index],    samplers[mat.occlusion_sampler_index]), uv).rgb;
    vec3 emission           = texture(sampler2D(textures[mat.emissive_map_index],     samplers[mat.emissive_sampler_index]),  uv).rgb;

    float metallic  = metallic_roughness.b;
    float roughness = metallic_roughness.g;

    normal = normalize(normal_map_normal * 2 - 1);
    normal = normalize(TBN * normal);

    // lighting
    vec3 light_pos = { 0,0,0 };
    vec3 light_color = { 1,1,1 };
    vec3 light_ambient = { 0,0,0 };

    vec3 color = calculate_lighting(position, normal, light_pos, light_color, light_ambient, diffuse_color, gl_WorldRayOriginEXT, roughness, metallic);

    p.color = vec4(light_ambient * occlusion + color + emission, 1);
}
