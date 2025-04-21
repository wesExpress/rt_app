#include "debug_pipeline.h"
#include "../app.h"

bool debug_pipeline_init(dm_context* context)
{
    application_data* app_data = context->app_data;

    // === vertex buffer ===
    {
        dm_vertex_buffer_desc desc = { 0 };
        desc.size         = sizeof(app_data->debug_data.vertices);
        desc.element_size = sizeof(debug_vertex);
        desc.stride       = sizeof(debug_vertex);
        //desc.data         = (void*)app_data->debug_data.vertices;

        if(!dm_renderer_create_vertex_buffer(desc, &app_data->debug_data.vb, context)) return false;
    }

    // === index buffer ===
    {
        dm_index_buffer_desc desc = { 0 };
        desc.size         = sizeof(app_data->debug_data.indices);
        desc.element_size = sizeof(uint32_t);
        desc.data         = NULL;
        desc.index_type   = DM_INDEX_BUFFER_INDEX_TYPE_UINT32;

        if(!dm_renderer_create_index_buffer(desc, &app_data->debug_data.ib, context)) return false;
    }

    // === raster pipeline ===
    {
        // input assembler
        dm_input_element_desc pos_element = {
            .name="POSITION", 
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_4, 
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX, 
            .stride=sizeof(debug_vertex), 
            .offset=offsetof(debug_vertex, position)
        };

        dm_input_element_desc color_element = {
            .name="COLOR",
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_4,
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
            .stride=sizeof(debug_vertex),
            .offset=offsetof(debug_vertex, color)
        };

        dm_raster_input_assembler_desc input_assembler = {
            .input_elements = { pos_element,color_element }, .input_element_count=2,
            .topology=DM_INPUT_TOPOLOGY_LINE_LIST
        };

        // rasterizer
        dm_rasterizer_desc rasterizer_desc = { 
            .cull_mode=DM_RASTERIZER_CULL_MODE_BACK, .polygon_fill=DM_RASTERIZER_POLYGON_FILL_FILL, .front_face=DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE,
#ifdef DM_DIRECTX12
            .vertex_shader_desc.path="assets/debug_vertex.cso",
            .pixel_shader_desc.path="assets/debug_pixel.cso",
#elif defined(DM_VULKAN)
            .vertex_shader_desc.path="assets/debug_shader.spv",
            .pixel_shader_desc.path="assets/debug_shader.spv",
#endif
        };

        // descriptors 
        dm_descriptor_group descriptor_group = {
            .descriptors = { DM_DESCRIPTOR_TYPE_CONSTANT_BUFFER },
            .count=1, .flags=DM_DESCRIPTOR_GROUP_FLAG_VERTEX_SHADER
        };

        dm_raster_pipeline_desc desc = { 
            .input_assembler=input_assembler,
            .rasterizer=rasterizer_desc,
            .descriptor_groups[0]=descriptor_group, .descriptor_group_count=1,
            .viewport.type=DM_VIEWPORT_TYPE_DEFAULT, .scissor.type=DM_SCISSOR_TYPE_DEFAULT,
            .depth_stencil=true
        };

        if(!dm_renderer_create_raster_pipeline(desc, &app_data->debug_data.pipeline, context)) return false;
    }
    return true;
}

void debug_pipeline_draw_line(dm_vec3 origin, dm_vec3 direction, float length, dm_vec4 color, dm_context* context)
{
    application_data* app_data = context->app_data;

    debug_vertex v1,v2;

    dm_vec3 t, d;

    dm_memcpy(v1.position, origin, sizeof(dm_vec3));
    dm_memcpy(v1.color, color, sizeof(dm_vec4));

    dm_vec3_scale(direction, length, d);
    dm_vec3_add_vec3(origin, d, v2.position);
    dm_memcpy(v2.color, color, sizeof(dm_vec4));

    app_data->debug_data.vertices[app_data->debug_data.vertex_count++] = v1;
    app_data->debug_data.vertices[app_data->debug_data.vertex_count++] = v2;

    uint32_t i = app_data->debug_data.index_count;
    app_data->debug_data.indices[app_data->debug_data.index_count++] = i; 
    app_data->debug_data.indices[app_data->debug_data.index_count++] = i+1; 
}

bool debug_pipeline_update(dm_context* context)
{
    application_data* app_data = context->app_data;
    
    dm_render_command_update_vertex_buffer(app_data->debug_data.vertices, sizeof(app_data->debug_data.vertices), app_data->debug_data.vb, context);
    dm_render_command_update_index_buffer(app_data->debug_data.indices, sizeof(app_data->debug_data.indices), app_data->debug_data.ib, context);

    return true;
}

bool debug_pipeline_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    if(app_data->debug_data.vertex_count)
    {
        dm_render_command_bind_raster_pipeline(app_data->debug_data.pipeline, context);
        dm_render_command_bind_vertex_buffer(app_data->debug_data.vb, 0, context);
        dm_render_command_bind_index_buffer(app_data->debug_data.ib, context);

        dm_render_command_bind_constant_buffer(app_data->raster_data.cb, 0,0, context);
        dm_render_command_bind_descriptor_group(0,1,0, context);

        dm_render_command_draw_instanced_indexed(app_data->debug_data.vertex_count / 2,0, app_data->debug_data.index_count,0, 0, context);
    }

    //
    app_data->debug_data.vertex_count = 0;
    app_data->debug_data.index_count  = 0;

    return true;
}
