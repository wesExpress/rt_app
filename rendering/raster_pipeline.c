#include "raster_pipeline.h"
#include "../app.h"
#include "../data.h"

#ifndef DM_DEBUG
DM_INLINE
#endif
bool create_mesh(const vertex* vertices, uint32_t vertex_count, const uint32_t* indices, uint32_t index_count, dm_resource_handle* v_handle, dm_resource_handle* i_handle, dm_context* context)
{
    dm_vertex_buffer_desc v_desc = {
        .size=sizeof(vertex) * vertex_count,
        .element_size=sizeof(float),
        .stride=sizeof(vertex),
        .data=(void*)vertices
    };

    dm_index_buffer_desc i_desc = {
        .size=sizeof(uint32_t) * index_count,
        .element_size=sizeof(uint32_t),
        .index_type=DM_INDEX_BUFFER_INDEX_TYPE_UINT32,
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
        desc.size = sizeof(dm_mat4);
        desc.data = app_data->camera.vp;

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

        // descriptors 
        dm_descriptor_group descriptor_group = {
            .descriptors = { DM_DESCRIPTOR_TYPE_CONSTANT_BUFFER, DM_DESCRIPTOR_TYPE_READ_STORAGE_BUFFER },
            .count=2, .flags=DM_DESCRIPTOR_GROUP_FLAG_VERTEX_SHADER
        };

        dm_raster_pipeline_desc desc = { 
            .input_assembler=input_assembler,
            .rasterizer=rasterizer_desc,
            .descriptor_groups[0]=descriptor_group, .descriptor_group_count=1,
            .viewport.type=DM_VIEWPORT_TYPE_DEFAULT, .scissor.type=DM_SCISSOR_TYPE_DEFAULT,
            .depth_stencil=true
        };

        if(!dm_renderer_create_raster_pipeline(desc, &app_data->raster_data.pipeline, context)) return false;
    }

    // === transform storage buffer ===
    {
        dm_storage_buffer_desc b_desc = { 0 };
        b_desc.write        = true;
        b_desc.size         = MAX_INSTANCES * sizeof(transform);
        b_desc.stride       = sizeof(transform);
        b_desc.data         = app_data->transforms;

        if(!dm_renderer_create_storage_buffer(b_desc, &app_data->raster_data.transform_sb, context)) return false;
    }

    // === instance storage buffer ===
    {
        dm_storage_buffer_desc b_desc = { 0 };
        b_desc.write        = true;
        b_desc.size         = MAX_INSTANCES * sizeof(instance);
        b_desc.stride       = sizeof(instance);
        b_desc.data         = NULL;

        if(!dm_renderer_create_storage_buffer(b_desc, &app_data->raster_data.instance_cb, context)) return false;
    }

    // === compute pipeline ===
    {
        dm_compute_pipeline_desc desc = { 0 };
#ifdef DM_DIRECTX12
        dm_strcpy(desc.shader.path, "assets/compute_shader.cso");
#elif defined(DM_VULKAN)
        dm_strcpy(desc.shader.path, "assets/compute_shader.spv");
#endif

        desc.descriptor_groups[0].descriptors[0]  = DM_DESCRIPTOR_TYPE_WRITE_STORAGE_BUFFER;
        desc.descriptor_groups[0].descriptors[1]  = DM_DESCRIPTOR_TYPE_WRITE_STORAGE_BUFFER;
        desc.descriptor_groups[0].count           = 2;
        desc.descriptor_groups[0].flags          |= DM_DESCRIPTOR_GROUP_FLAG_COMPUTE_SHADER;
        desc.descriptor_group_count = 1;

        if(!dm_compute_create_compute_pipeline(desc, &app_data->raster_data.compute_pipeline, context)) return false;
    }

    return true;
}

bool raster_pipeline_update(dm_context* context)
{
    return true;
}

bool raster_pipeline_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    // update the transforms and compute model matrices
    dm_compute_command_bind_compute_pipeline(app_data->raster_data.compute_pipeline, context);
    dm_compute_command_bind_storage_buffer(app_data->raster_data.transform_sb, 0,0, context);
    dm_compute_command_bind_storage_buffer(app_data->raster_data.instance_cb, 1,0, context);
    dm_compute_command_bind_descriptor_group(0,2,0, context);
    dm_compute_command_dispatch(1024,1,1, context);

    // object rendering
    dm_mat4_transpose(app_data->camera.vp, app_data->camera.vp);
    dm_render_command_update_constant_buffer(app_data->camera.vp, sizeof(dm_mat4), app_data->raster_data.cb, context);

    dm_render_command_bind_raster_pipeline(app_data->raster_data.pipeline, context);
    dm_render_command_bind_constant_buffer(app_data->raster_data.cb, 0,0, context);
    dm_render_command_bind_storage_buffer(app_data->raster_data.instance_cb, 1,0, context);
    dm_render_command_bind_descriptor_group(0,2,0, context);

    dm_render_command_bind_vertex_buffer(app_data->raster_data.vb_tri, 0, context);
    dm_render_command_bind_index_buffer(app_data->raster_data.ib_tri, context);
    dm_render_command_draw_instanced_indexed(MAX_INSTANCES,0, _countof(triangle_indices),0, 0, context);

    return true;
}
