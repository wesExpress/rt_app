#version 450

struct transform
{
    vec4 position;
    vec4 scale;
    vec4 orientation;
};

struct instance
{
    mat4 model;
    mat4 normal;
};

layout(set=0, binding=0) buffer transform_buffer
{
    transform transforms[];
};

layout(set=0, binding=1) writeonly buffer instance_buffer 
{
    instance instances[];
};

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

layout(local_size_x=64, local_size_y=1, local_size_z=1) in;
void main()
{
    uint index = gl_GlobalInvocationID.x;

    transform t = transforms[index];

    mat4 model = IDENTITY;

    mat4 translation = matrix_translate(t.position);
    mat4 rotation    = matrix_rotate(t.orientation);
    mat4 scaling     = matrix_scale(t.scale);

    model = scaling * rotation * translation; 

    vec3 quat_rotate = vec3(0,0.001f,0);
    transforms[index].orientation = update_orientation(t.orientation, quat_rotate);
    instances[index].model        = transpose(model);
}

