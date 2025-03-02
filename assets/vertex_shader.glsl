#version 450

layout(set=0, binding=0) uniform UniformBufferObject
{
    mat4 view_projection;
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_color;
layout(location = 3) in mat4 obj_model;
layout(location = 7) in mat4 obj_normal;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec4 out_color;

void main()
{
    vec4 p = vec4(position.x, position.y, position.z, 1.f);
    
    gl_Position = ubo.view_projection * obj_model * p;

    out_color = in_color;
}

