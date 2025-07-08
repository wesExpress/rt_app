#include "raster_pipeline.h"
#include "../app.h"
#include "../data.h"

#ifndef DM_DEBUG
DM_INLINE
#endif
bool create_mesh(const vertex* vertices, uint32_t vertex_count, const index_t* indices, uint32_t index_count, dm_resource_handle* v_handle, dm_resource_handle* i_handle, dm_context* context)
{
    dm_vertex_buffer_desc v_desc = {
        .size=sizeof(vertex) * vertex_count,
        .element_size=sizeof(float),
        .stride=sizeof(vertex),
        .data=(void*)vertices
    };

    dm_index_buffer_desc i_desc = {
        .size=sizeof(index_t) * index_count,
        .element_size=sizeof(index_t),
        .index_type=INDEX_TYPE,
        .data=(void*)indices
    };

    if(!dm_renderer_create_vertex_buffer(v_desc, v_handle, context)) return false;
    if(!dm_renderer_create_index_buffer(i_desc, i_handle, context)) return false;

    return true;
}
#define CREATE_MESH(VERTICES, INDICES, V_HANDLE, I_HANDLE, CONTEXT) create_mesh(VERTICES, _countof(VERTICES), INDICES, _countof(INDICES), V_HANDLE, I_HANDLE, CONTEXT)

bool raster_pipeline_init(dm_context* context)
{
    application_data* app_data = context->app_data;

    // === vertex and index buffers ===
    {
        if(!CREATE_MESH(cube, cube_indices, &app_data->raster_data.vb_cube, &app_data->raster_data.ib_cube, context)) return false;
        if(!CREATE_MESH(triangle, triangle_indices, &app_data->raster_data.vb_tri, &app_data->raster_data.ib_tri, context)) return false;
        if(!CREATE_MESH(quad, quad_indices, &app_data->raster_data.vb_quad, &app_data->raster_data.ib_quad, context)) return false;
    }

    // === constant buffer ===
    {
        dm_constant_buffer_desc desc = { 0 };
        desc.size = sizeof(raster_scene_data);
        desc.data = &app_data->raster_data.scene_data;

        if(!dm_renderer_create_constant_buffer(desc, &app_data->raster_data.cb, context)) return false;
    }

    // === raster pipeline ===
    {
        // input assembler
        dm_input_element_desc pos_element = {
            .name="POSITION", 
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_4, 
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX, 
            .stride=sizeof(vertex), 
            .offset=offsetof(vertex, pos)
        };

        dm_input_element_desc normal_element = {
            .name="NORMAL",
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_4,
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
            .stride=sizeof(vertex),
            .offset=offsetof(vertex, normal)
        };

        dm_input_element_desc color_element = {
            .name="COLOR",
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_4,
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
            .stride=sizeof(vertex),
            .offset=offsetof(vertex, color)
        };

        dm_raster_input_assembler_desc input_assembler = {
            .input_elements = { pos_element,normal_element,color_element }, .input_element_count=3,
            .topology=DM_INPUT_TOPOLOGY_TRIANGLE_LIST
        };

        // rasterizer
        dm_rasterizer_desc rasterizer_desc = { 
            .cull_mode=DM_RASTERIZER_CULL_MODE_BACK, .polygon_fill=DM_RASTERIZER_POLYGON_FILL_FILL, .front_face=DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE,
#ifdef DM_DIRECTX12
            .vertex_shader_desc.path="assets/vertex_shader.cso",
            .pixel_shader_desc.path="assets/pixel_shader.cso",
#elif defined(DM_VULKAN)
            .vertex_shader_desc.path="assets/vertex_shader.spv",
            .pixel_shader_desc.path="assets/pixel_shader.spv",
#endif
        };

        dm_raster_pipeline_desc desc = { 
            .input_assembler=input_assembler,
            .rasterizer=rasterizer_desc,
            .viewport.type=DM_VIEWPORT_TYPE_DEFAULT, .scissor.type=DM_SCISSOR_TYPE_DEFAULT,
            .depth_stencil=true
        };

        if(!dm_renderer_create_raster_pipeline(desc, &app_data->raster_data.pipeline, context)) return false;
    }

    return true;
}

bool raster_pipeline_update(dm_context* context)
{
    application_data* app_data = context->app_data;

    app_data->raster_data.c_data.delta_t = 0.001f;
    
#ifdef DM_DIRECTX12
    dm_mat4_transpose(app_data->camera.vp, app_data->raster_data.scene_data.view_proj);
#else
    dm_memcpy(app_data->raster_data.scene_data.view_proj, app_data->camera.vp, sizeof(dm_mat4));
#endif

    return true;
}

bool raster_pipeline_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    app_data->raster_data.render_data.scene_cb        = app_data->raster_data.cb.descriptor_index;
    app_data->raster_data.render_data.instance_buffer = app_data->entities.instance_sb.descriptor_index;

    dm_render_command_update_constant_buffer(&app_data->raster_data.scene_data, sizeof(raster_scene_data), app_data->raster_data.cb, context);
    dm_render_command_bind_raster_pipeline(app_data->raster_data.pipeline, context);
    dm_render_command_set_root_constants(0,2,0, &app_data->raster_data.render_data, context);
    dm_render_command_bind_vertex_buffer(app_data->raster_data.vb_cube, 0, context);
    dm_render_command_bind_index_buffer(app_data->raster_data.ib_cube, context);
    dm_render_command_draw_instanced_indexed(MAX_ENTITIES,0, _countof(cube_indices),0, 0, context);

    return true;
}
