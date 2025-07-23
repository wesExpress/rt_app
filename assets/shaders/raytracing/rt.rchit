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
    uint is_indexed;
    uint material_indices[3];
    uint sampler_index;
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
    uint image_index;
    uint camera_data_index;
    uint scene_data_index;
    uint material_buffer_index;
};

layout(set=0, binding=0) uniform accelerationStructureEXT acceleration_structures[RESOURCE_HEAP_SIZE];

layout(set=0, binding=0) buffer b0 { material data[]; } material_buffers[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) buffer b1 { vertex data[]; }   vertex_buffers[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) buffer b2 { uint data[]; }     index_buffers[RESOURCE_HEAP_SIZE];

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

void get_vertices_indexed(material m, inout vertex vertices[3])
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

    material m = material_buffers[material_buffer_index].data[inst_index];

    vertex vertices[3];

    get_vertices_indexed(m, vertices);

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

    vec3 diffuse_color = texture(sampler2D(textures[m.material_indices[0]], samplers[m.sampler_index]), uv).rgb; 

    normal = texture(sampler2D(textures[m.material_indices[1]], samplers[m.sampler_index]), uv).rgb;
    normal = normalize(normal * 2 - 1);
    normal = normalize(normal * TBN);

    vec3 dir = normalize(light - position);
    float diff = max(dot(normal, dir), 0);

    p.color.rgb = diffuse_color * diff;
    //p.color.rgb = position;
}
