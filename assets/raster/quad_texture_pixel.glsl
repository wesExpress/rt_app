#version 460

layout(location=0) in vec2 fragment_tex_coords;

layout(location=0) out vec4 pixel_color;

layout(set=0, binding=3) uniform texture2D textures[100];
layout(set=1, binding=0) uniform sampler   samplers[100];

layout(push_constant) uniform render_resources
{
    uint image_index;
    uint sampler_index;
};

void main()
{
    pixel_color = texture(sampler2D(textures[image_index], samplers[sampler_index]), fragment_tex_coords);
}
