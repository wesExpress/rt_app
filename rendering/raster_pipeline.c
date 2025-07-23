#include "raster_pipeline.h"
#include "../app.h"

typedef struct raster_vertex_t
{
    dm_vec4 position;
    dm_vec4 normal;
    dm_vec4 tangent;
    dm_vec4 color;
} raster_vertex;

bool raster_pipeline_init(dm_context* context)
{
    application_data* app_data = context->app_data;

    // === vertex and index buffers ===
    {
        dm_vertex_buffer_desc desc = { 0 };
        desc.stride = sizeof(raster_inst);
        desc.size   = sizeof(app_data->raster_data.instances);
        desc.data   = NULL;

        if(!dm_renderer_create_vertex_buffer(desc, &app_data->raster_data.inst_vb, context)) return false;
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
            .stride=sizeof(raster_vertex), 
            .offset=offsetof(raster_vertex, position)
        };

        dm_input_element_desc normal_element = {
            .name="NORMAL",
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_4,
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
            .stride=sizeof(raster_vertex),
            .offset=offsetof(raster_vertex, normal)
        };

        dm_input_element_desc tangent_element = {
            .name="TANGENT",
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_4,
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
            .stride=sizeof(raster_vertex),
            .offset=offsetof(raster_vertex, tangent)
        };

        dm_input_element_desc color_element = {
            .name="COLOR",
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_4,
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
            .stride=sizeof(raster_vertex),
            .offset=offsetof(raster_vertex, color)
        };

        dm_input_element_desc model_element = {
            .name="MODEL",
            .format=DM_INPUT_ELEMENT_FORMAT_MATRIX_4x4,
            .class=DM_INPUT_ELEMENT_CLASS_PER_INSTANCE,
            .stride=sizeof(raster_inst),
            .offset=offsetof(raster_inst, model)
        };

        dm_raster_input_assembler_desc input_assembler = {
            .input_elements = { pos_element,normal_element,tangent_element,color_element,model_element }, .input_element_count=5,
            .topology=DM_INPUT_TOPOLOGY_TRIANGLE_LIST
        };

        // rasterizer
        dm_rasterizer_desc rasterizer_desc = { 
            .cull_mode=DM_RASTERIZER_CULL_MODE_BACK, .polygon_fill=DM_RASTERIZER_POLYGON_FILL_FILL, .front_face=DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE,
#ifdef DM_DIRECTX12
            .vertex_shader_desc.path="assets/shaders/vertex_shader.cso",
            .pixel_shader_desc.path="assets/shaders/pixel_shader.cso",
#elif defined(DM_VULKAN)
            .vertex_shader_desc.path="assets/shaders/vertex_shader.spv",
            .pixel_shader_desc.path="assets/shaders/pixel_shader.spv",
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

    dm_render_command_update_constant_buffer(&app_data->raster_data.scene_data, sizeof(raster_scene_data), app_data->raster_data.cb, context);

    return true;
}

void raster_pipeline_render(dm_mesh mesh, uint32_t count, dm_context* context)
{
    if(count==0) return;

    application_data* app_data = context->app_data;

    app_data->raster_data.render_data.scene_cb              = app_data->raster_data.cb.descriptor_index;
    app_data->raster_data.render_data.material_buffer_index = app_data->entities.material_sb.descriptor_index;

    dm_render_command_bind_raster_pipeline(app_data->raster_data.pipeline, context);
    dm_render_command_set_root_constants(0,4,0, &app_data->raster_data.render_data, context);
    dm_render_command_bind_vertex_buffer(mesh.vb, 0, context);
    dm_render_command_bind_vertex_buffer(app_data->raster_data.inst_vb, 1, context);

    if(mesh.index_count)
    {
        dm_render_command_bind_index_buffer(mesh.ib, context);
        dm_render_command_draw_instanced_indexed(count,0, mesh.index_count,0, 0, context);
    }
    else
    {
        dm_render_command_draw_instanced(count,0,mesh.vertex_count,0, context);
    }
}
