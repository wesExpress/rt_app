#include "raster_pipeline.h"
#include "../app.h"
#include "../data.h"

bool raster_pipeline_init(dm_context* context)
{
    application_data* app_data = context->app_data;

    // === vertex buffer ===
    {
        dm_vertex_buffer_desc desc = { 0 };
        desc.size         = sizeof(cube);
        desc.element_size = sizeof(float);
        desc.stride       = sizeof(vertex);
        desc.data         = (void*)cube;

        if(!dm_renderer_create_vertex_buffer(desc, &app_data->raster_data.vb, context)) return false;
    }

    // === index buffer ===
    {
        dm_index_buffer_desc desc = { 0 };
        desc.size         = sizeof(cube_indices);
        desc.element_size = sizeof(uint32_t);
        desc.data         = (void*)cube_indices;
        desc.index_type   = INDEX_TYPE;

        if(!dm_renderer_create_index_buffer(desc, &app_data->raster_data.ib, context)) return false;
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
        dm_raster_pipeline_desc desc = { 0 };
       
        // input assembler
        dm_input_element_desc* input = desc.input_assembler.input_elements;
        dm_strcpy(input->name, "POSITION");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_3;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(vertex);
        input->offset = offsetof(vertex, pos);

        input++;

        dm_strcpy(input->name, "NORMAL");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_3;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(vertex);
        input->offset = offsetof(vertex, normal);

        input++;

        dm_strcpy(input->name, "COLOR");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_4;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(vertex);
        input->offset = offsetof(vertex, color);

        input++;

        desc.input_assembler.input_element_count = 3;

        desc.input_assembler.topology = DM_INPUT_TOPOLOGY_TRIANGLE_LIST;

        // rasterizer
        desc.rasterizer.cull_mode    = DM_RASTERIZER_CULL_MODE_BACK;
        desc.rasterizer.polygon_fill = DM_RASTERIZER_POLYGON_FILL_FILL;
        desc.rasterizer.front_face   = DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE;

        // shaders
#ifdef DM_DIRECTX12
        dm_strcpy(desc.rasterizer.vertex_shader_desc.path, "assets/vertex_shader.cso");
        dm_strcpy(desc.rasterizer.pixel_shader_desc.path,  "assets/pixel_shader.cso");
#elif defined(DM_VULKAN)
        dm_strcpy(desc.rasterizer.vertex_shader_desc.path, "assets/vertex_shader.spv");
        dm_strcpy(desc.rasterizer.pixel_shader_desc.path,  "assets/pixel_shader.spv");
#endif

        // viewport and scissor
        desc.viewport.type = DM_VIEWPORT_TYPE_DEFAULT;
        desc.scissor.type  = DM_SCISSOR_TYPE_DEFAULT;

        // descriptors 
        desc.descriptor_groups[0].descriptors[0]  = DM_DESCRIPTOR_TYPE_CONSTANT_BUFFER;
        desc.descriptor_groups[0].descriptors[1]  = DM_DESCRIPTOR_TYPE_READ_STORAGE_BUFFER;
        desc.descriptor_groups[0].count           = 2;
        desc.descriptor_groups[0].flags          |= DM_DESCRIPTOR_GROUP_FLAG_VERTEX_SHADER;

        desc.descriptor_group_count = 1;

        // depth stencil
        desc.depth_stencil.depth = true;

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
    dm_render_command_update_constant_buffer(app_data->camera.vp, sizeof(dm_mat4), app_data->raster_data.cb, context);

    dm_render_command_bind_raster_pipeline(app_data->raster_data.pipeline, context);
    dm_render_command_bind_constant_buffer(app_data->raster_data.cb, 0,0, context);
    dm_render_command_bind_storage_buffer(app_data->raster_data.instance_cb, 1,0, context);
    dm_render_command_bind_descriptor_group(0,2,0, context);

    dm_render_command_bind_vertex_buffer(app_data->raster_data.vb, 0, context);
    dm_render_command_bind_index_buffer(app_data->raster_data.ib, context);

    dm_render_command_draw_instanced_indexed(MAX_INSTANCES,0, _countof(cube_indices),0, 0, context);

    return true;
}
