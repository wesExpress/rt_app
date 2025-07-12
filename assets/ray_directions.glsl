#version 460

layout(set=0, binding=0) uniform camera_uniforms
{
    mat4 inv_view;
    mat4 inv_proj;
    vec4 position;
} camera_uniform[100];

layout(set=0, binding=0) uniform image_uniforms
{
    uint width;
    uint height;
} image_uniform[100];

layout(set=0, binding=2) uniform writeonly image2D textures[100];

layout(push_constant) uniform render_resources
{
    uint camera_data;
    uint image_data;
    uint ray_image;
};

layout(local_size_x=8, local_size_y=8, local_size_z=1) in;
void main()
{
    mat4 inv_view = camera_uniform[camera_data].inv_view;
    mat4 inv_proj = camera_uniform[camera_data].inv_proj;
    vec4 camera_position = camera_uniform[camera_data].position;

    const uvec2 index             = gl_GlobalInvocationID.xy;
    const uvec2 screen_dimensions = uvec2(image_uniform[image_data].width, image_uniform[image_data].height); 
    const float aspect_ratio      = float(screen_dimensions.x) / float(screen_dimensions.y);

    // get screen position
    const vec2 ndc  = (vec2(index) + 0.5f) / vec2(screen_dimensions);
    vec2 screen_pos = ndc * 2.f - 1.f;
    screen_pos.y *= -1.f;

    // world position
    vec3 world_pos = (inv_proj * vec4(screen_pos, -1,1)).xyz;
    world_pos      = (inv_view * vec4(world_pos, 1)).xyz;

    // direction
    vec4 direction = vec4(normalize(world_pos - camera_position.xyz), 1);
    direction = vec4(1,0,1,1);

    // write out
    imageStore(textures[ray_image], ivec2(index), direction);
}
