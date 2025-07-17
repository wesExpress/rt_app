#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "shader_include.h"

struct transform
{
    vec4 position;
    vec4 scale;
    vec4 orientation;
};

struct instance
{
    mat4 model;
};

struct rt_instance
{
    vec4    transform[3];

    uint id_mask;
    uint offset_flags;

    uint64_t blas;
};

struct physics
{
    vec4 w;
};

layout(push_constant) uniform render_resources
{
    uint transform_b;
    uint physics_b;
    uint instance_b;
    uint rt_b;
    uint blas_b;
};

layout(set=0, binding=0) buffer b0 { transform data[]; }   transform_buffer[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) buffer b1 { instance data[]; }    instance_buffer[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) buffer b2 { physics data[]; }     physics_buffer[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) buffer b3 { rt_instance data[]; } rt_instance_buffer[RESOURCE_HEAP_SIZE];
layout(set=0, binding=0) buffer b4 { uint64_t data[]; }    rt_blas_buffer[RESOURCE_HEAP_SIZE];

#define IDENTITY mat4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)

mat4 matrix_translate(vec4 position)
{
    mat4 translation = IDENTITY;

    translation[0][3] = position.x;
    translation[1][3] = position.y;
    translation[2][3] = position.z;

    return translation;
}

mat4 matrix_scale(vec4 scale)
{
    mat4 scaling = IDENTITY;

    scaling[0][0] = scale.x;
    scaling[1][1] = scale.y;
    scaling[2][2] = scale.z;

    return scaling;
}

mat4 matrix_rotate(vec4 orientation)
{
    mat4 rotation = IDENTITY;

    const float x = orientation[0];
    const float y = orientation[1];
    const float z = orientation[2];
    const float w = orientation[3];

    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;
    
    const float xy = x * y;
    const float xz = x * z;
    const float xw = x * w;
    const float yz = y * z;
    const float yw = y * w;
    const float zw = z * w;
    
    rotation[0][0] = 1 - 2 * (yy + zz);
    rotation[0][1] = 2 * (xy - zw);
    rotation[0][2] = 2 * (xz + yw);
    
    rotation[1][0] = 2 * (xy + zw);
    rotation[1][1] = 1 - 2 * (xx + zz);
    rotation[1][2] = 2 * (yz - xw);
    
    rotation[2][0] = 2 * (xz - yw);
    rotation[2][1] = 2 * (yz + xw);
    rotation[2][2] = 1 - 2 * (xx + yy);

    return rotation;
}

vec4 update_orientation(vec4 orientation, vec3 rotation)
{
    vec4 new_orientation;

    new_orientation.x = orientation.x + rotation.x * orientation.w + rotation.y * orientation.z - rotation.z * orientation.y;
    new_orientation.y = orientation.y - rotation.x * orientation.z + rotation.y * orientation.w + rotation.z * orientation.x;
    new_orientation.z = orientation.z + rotation.x * orientation.y - rotation.y * orientation.x + rotation.z * orientation.w;
    new_orientation.w = orientation.w - rotation.x * orientation.x - rotation.y * orientation.y - rotation.z * orientation.z;

    return normalize(new_orientation);
}

layout(local_size_x=8, local_size_y=1, local_size_z=1) in;
void main()
{
    uint index = gl_GlobalInvocationID.x;

    transform t = transform_buffer[transform_b].data[index];
    instance  i = instance_buffer[instance_b].data[index];
    physics   p = physics_buffer[physics_b].data[index];

    mat4 model = IDENTITY;

    mat4 translation = matrix_translate(t.position);
    mat4 rotation    = matrix_rotate(t.orientation);
    mat4 scaling     = matrix_scale(t.scale);

    model = scaling * rotation * translation * model;


    rt_instance_buffer[rt_b].data[index].transform[0] = model[0];
    rt_instance_buffer[rt_b].data[index].transform[1] = model[1];
    rt_instance_buffer[rt_b].data[index].transform[2] = model[2];

    rt_instance_buffer[rt_b].data[index].id_mask = index;
    rt_instance_buffer[rt_b].data[index].id_mask |= 0xFF << 24;

    rt_instance_buffer[rt_b].data[index].blas = rt_blas_buffer[blas_b].data[0];

    uint count = 2;
    const float half_dt = 0.0016f * 0.5f;
    vec3 delta_rot = p.w.xyz * half_dt;
    
    while(count > 0)
    {
        t.orientation = update_orientation(t.orientation, delta_rot);
        count--;
    }

    model = transpose(model);
    transform_buffer[transform_b].data[index].orientation = t.orientation;
    instance_buffer[instance_b].data[index].model         = model;
}

