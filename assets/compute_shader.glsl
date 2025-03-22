#version 450

layout(set=0, binding=0) readonly buffer src_buffer
{
    int data_in[];
};

layout(set=0, binding=1) writeonly buffer dest_buffer
{
    int data_out[];
};

layout(local_size_x=64, local_size_y=1, local_size_z=1) in;
void main()
{
    uint index = gl_GlobalInvocationID.x;

    data_out[index] = data_in[index] + int(index) + 11;
}
