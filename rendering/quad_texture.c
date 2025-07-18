#include "quad_texture.h"
#include "../app.h"

typedef struct quad_vertex_t
{
    float position[2];
    float tex_coords[2];
} quad_vertex;

bool quad_texture_init(dm_context* context)
{
    application_data* app_data = context->app_data;

    quad_vertex vertices[] = {
        { { -1.f,-1.f }, { 0,1 } },
        { {  1.f,-1.f }, { 1,1 } },
        { {  1.f, 1.f }, { 1,0 } },
        { { -1.f, 1.f }, { 0,0 } },
    };

    uint32_t indices[] = {
        0,1,2,
        2,3,0
    };

    // === vertex buffer ===
    {
        dm_vertex_buffer_desc desc = { 
            .size=sizeof(vertices), .data=vertices,
            .element_size=sizeof(float), .stride=sizeof(quad_vertex)
        };

        if(!dm_renderer_create_vertex_buffer(desc, &app_data->quad_texture_data.vb, context)) return false;
    }

    // === index buffer ===
    {
        dm_index_buffer_desc desc = { 
            .size=sizeof(indices), .data=indices,
            .index_type=DM_INDEX_BUFFER_INDEX_TYPE_UINT32, .element_size=sizeof(uint32_t)
        };

        if(!dm_renderer_create_index_buffer(desc, &app_data->quad_texture_data.ib, context)) return false;
    };

    // === pipeline ===
    {
        dm_shader_desc vertex_shader_desc = {
#ifdef DM_DIRECTX12
            .path="assets/quad_texture_vertex.cso",
#elif defined(DM_VULKAN)
            .path="assets/quad_texture_vertex.spv"
#endif
        };

        dm_shader_desc pixel_shader_desc = {
#ifdef DM_DIRECTX12
            .path="assets/quad_texture_pixel.cso",
#elif defined(DM_VULKAN)
            .path="assets/quad_texture_pixel.spv"
#endif
        };

        dm_rasterizer_desc raster_desc = {
            .cull_mode=DM_RASTERIZER_CULL_MODE_BACK, .front_face=DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE, .polygon_fill=DM_RASTERIZER_POLYGON_FILL_FILL,
            .vertex_shader_desc=vertex_shader_desc, .pixel_shader_desc=pixel_shader_desc
        };

        dm_input_element_desc position_element = {
            .name="POSITION",
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_2,
            .offset=offsetof(quad_vertex, position),
            .stride=sizeof(quad_vertex)
        };

        dm_input_element_desc tex_coords_element = {
            .name="TEX_COORDS",
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_2,
            .offset=offsetof(quad_vertex, tex_coords),
            .stride=sizeof(quad_vertex)
        };

        dm_raster_input_assembler_desc input_assembler = {
            .input_elements={ position_element, tex_coords_element }, .input_element_count=2,
            .topology=DM_INPUT_TOPOLOGY_TRIANGLE_LIST
        };

        dm_viewport viewport = {
            .type=DM_VIEWPORT_TYPE_DEFAULT
        };

        dm_scissor scissor = {
            .type=DM_SCISSOR_TYPE_DEFAULT
        };

        dm_raster_pipeline_desc desc = {
            .rasterizer=raster_desc,
            .input_assembler=input_assembler,
            .viewport=viewport, .scissor=scissor
        };

        if(!dm_renderer_create_raster_pipeline(desc, &app_data->quad_texture_data.pipeline, context)) return false;
    }

    return true;
}

void quad_texture_render(dm_resource_handle texture, dm_context* context)
{
    application_data* app_data = context->app_data;

    app_data->quad_texture_data.resources.image_index   = texture.descriptor_index + 1; 
    app_data->quad_texture_data.resources.sampler_index = app_data->default_sampler.descriptor_index;

    dm_render_command_bind_raster_pipeline(app_data->quad_texture_data.pipeline, context);
    dm_render_command_set_root_constants(0,2,0, &app_data->quad_texture_data.resources, context);
    dm_render_command_bind_vertex_buffer(app_data->quad_texture_data.vb, 0, context);
    dm_render_command_bind_index_buffer(app_data->quad_texture_data.ib, context);

    dm_render_command_draw_instanced_indexed(1,0,6,0,0,context);
} 
