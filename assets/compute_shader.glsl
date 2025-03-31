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

layout(set=0, binding=0) readonly buffer transform_buffer
{
    transform transforms[];
};

layout(set=0, binding=1) writeonly buffer instance_buffer 
{
    instance instances[];
};

#define IDENTITY mat4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)

mat4 translate(mat4 model, vec4 position)
{
    model[0][3] = position.x;
    model[1][3] = position.y;
    model[2][3] = position.z;

    return model;
}

layout(local_size_x=64, local_size_y=1, local_size_z=1) in;
void main()
{
    uint index = gl_GlobalInvocationID.x;

    transform t = transforms[index];
    mat4 model = IDENTITY;

    model = translate(model, t.position);

    instances[index].model = transpose(model);
}
