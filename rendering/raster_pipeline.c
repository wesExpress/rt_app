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

        dm_raster_input_assembler_desc input_assembler = {
            .input_elements = { pos_element,normal_element,tangent_element,color_element }, .input_element_count=4,
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

void raster_pipeline_render(dm_scene scene, dm_context* context)
{
    application_data* app_data = context->app_data;

    app_data->raster_data.render_data.scene_cb              = app_data->raster_data.cb.descriptor_index;
    app_data->raster_data.render_data.material_buffer_index = app_data->material_sb.descriptor_index;
    app_data->raster_data.render_data.mesh_buffer_index     = app_data->mesh_sb.descriptor_index;
    app_data->raster_data.render_data.node_buffer_index     = app_data->node_buffer.descriptor_index;
    app_data->raster_data.render_data.light_buffer_index    = app_data->light_buffer.descriptor_index;

    dm_render_command_bind_raster_pipeline(app_data->raster_data.pipeline, context);
    for(uint32_t i=0; i<scene.node_count; i++)
    {
        dm_render_command_set_root_constants(0,5,0, &app_data->raster_data.render_data, context);
        dm_render_command_bind_vertex_buffer(scene.meshes[scene.nodes[i].mesh_index].vb, 0, context);
        dm_render_command_bind_index_buffer(scene.meshes[scene.nodes[i].mesh_index].ib, context);

        dm_render_command_draw_instanced_indexed(1,0,scene.meshes[scene.nodes[i].mesh_index].vertex_count,0,0, context);
    }
}
