#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_ARB_shading_language_include : enable

#include "..\shader_include.h"

struct payload
{
    vec4 color;
};

struct material
{
    uint vb_index;
    uint ib_index;
};

struct vertex
{
    vec4 position;
    vec4 normal;
    vec4 color;
};

layout(push_constant) uniform render_resources
{
    uint acceleration_structure_index;
    uint image_index;
    uint camera_data_index;
    uint scene_data_index;
    uint material_buffer_index;
};

layout(set=0, binding=0) uniform accelerationStructureEXT acceleration_structures[RESOURCE_HEAP_SIZE];

layout(set=0, binding=0) buffer b0 { material data[]; } material_buffers[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) buffer b1 { vertex data[]; }   vertex_buffers[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) buffer b2 { uint data[]; }     index_buffers[RESOURCE_HEAP_SIZE];

layout(location=0) rayPayloadInEXT payload p;
hitAttributeEXT vec2 attribs;

#if 0
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
#endif
vec3 get_barycentrics(vec2 attribs)
{
    return vec3(
        1 - attribs.x - attribs.y,
        attribs.x,
        attribs.y
    );
}

vec3 interpolate_value(vec3 values[3], vec3 barycentrics)
{
    return values[0] * barycentrics.x +
           values[1] * barycentrics.y +
           values[2] * barycentrics.z;
}

void get_vertices(material m, inout vertex vertices[3])
{
    uint tri_index = gl_PrimitiveID * 3;

    vertices[0] = vertex_buffers[m.vb_index].data[tri_index + 0];
    vertices[1] = vertex_buffers[m.vb_index].data[tri_index + 1];
    vertices[2] = vertex_buffers[m.vb_index].data[tri_index + 2];
}

void main()
{
    const vec3 light = vec3(0);

    uint inst_index = gl_InstanceCustomIndexEXT;
    uint tri_index  = gl_PrimitiveID * 3;

    material m = material_buffers[material_buffer_index].data[inst_index];

    vertex vertices[3];

    get_vertices(m, vertices);

    vec3 positions[3] = {
        vertices[0].position.xyz,
        vertices[1].position.xyz,
        vertices[2].position.xyz
    };

    vec3 normals[3] = {
        vertices[0].normal.xyz,
        vertices[1].normal.xyz,
        vertices[2].normal.xyz
    };

    vec3 colors[3] = {
        vertices[0].color.rgb,
        vertices[1].color.rgb,
        vertices[2].color.rgb
    };

    vec3 barycentrics = get_barycentrics(attribs);

    vec3 position = interpolate_value(positions, barycentrics);
    vec3 color    = interpolate_value(colors, barycentrics);

    position = (gl_ObjectToWorldEXT * vec4(position, 1)).xyz;

    vec3 a = (gl_ObjectToWorldEXT * vec4(positions[0], 1)).xyz;
    vec3 b = (gl_ObjectToWorldEXT * vec4(positions[1], 1)).xyz;
    vec3 c = (gl_ObjectToWorldEXT * vec4(positions[2], 1)).xyz;

    vec3 ab = b - a;
    vec3 ac = c - a;

    vec3 normal = normalize(cross(ab, ac));

    p.color.rgb = color * dot(light - position, normal);
}
