#version 450

layout(location = 0) in vec3 position;

void main()
{
    gl_Position.x = position.x;
    gl_Position.y = -position.y;
    gl_Position.z = position.z;
    gl_Position.w = 1.f;
}

