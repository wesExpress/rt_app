#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_ARB_shading_language_include : enable

#include "..\shader_include.h"

struct payload
{
    vec4 color;
};

struct camera_data
{
    mat4 inv_view, inv_proj;
    vec4 origin;
};

struct scene_data
{
    vec4 sky_color;
};

layout(push_constant) uniform render_resources
{
    uint acceleration_structure_index;
    uint image_index;
    uint camera_data_index;
    uint scene_data_index;
    uint material_buffer_index;
};

layout(set=0, binding=0) uniform u1 { scene_data data; }  scene_uniform[RESOURCE_HEAP_SIZE];

layout(location=0) rayPayloadInEXT payload p;

void main()
{
    p.color = scene_uniform[scene_data_index].data.sky_color;
}
