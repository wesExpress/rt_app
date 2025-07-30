#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_ARB_shading_language_include : enable

#include "..\shader_include.h"

struct shadow_payload
{
    float attenuation;
};

layout(location=1) rayPayloadInEXT shadow_payload p;

void main()
{
    p.attenuation = 1.f;
}
