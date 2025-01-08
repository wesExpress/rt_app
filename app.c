#include "dm.h"

typedef struct vertex_t
{
    float pos[3];
} vertex;

typedef struct application_data_t
{
    dm_render_handle raster_pipe;
    dm_render_handle vb;

    dm_timer frame_timer;
    uint16_t frame_count;
} application_data;

const vertex triangle[] = {
    { -0.5f,-0.5f,0.f },
    {  0.5f,-0.5f,0.f },
    {  0.f,  0.5f,0.f }
};

void dm_application_setup(dm_context_init_packet* init_packet)
{
    init_packet->app_data_size = sizeof(application_data);
}

bool dm_application_init(dm_context* context)
{
    application_data* app_data = context->app_data;

    // === raster pipeline ===
    {
        dm_raster_pipeline_desc desc = { 0 };
       
        // input assembler
        dm_input_element_desc* input = &desc.input_assembler.input_elements[0];
        dm_strcpy(input->name, "POSITION");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_3;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(vertex);

        desc.input_assembler.topology = DM_INPUT_TOPOLOGY_TRIANGLE_LIST;

        desc.input_assembler.input_element_count = 1;

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

        if(!dm_renderer_create_raster_pipeline(desc, &app_data->raster_pipe, context)) return false;
    }

    // === vertex buffer ===
    {
        dm_vertex_buffer_desc desc = { 0 };
        desc.size         = sizeof(triangle);
        desc.element_size = sizeof(float);
        desc.stride       = sizeof(vertex);
        desc.data         = (void*)triangle;

        if(!dm_renderer_create_vertex_buffer(desc, &app_data->vb, context)) return false;
    }

    // misc
    dm_timer_start(&app_data->frame_timer, context);

    return true;
}

void dm_application_shutdown(dm_context* context)
{
}

bool dm_application_update(dm_context* context)
{
    application_data* app_data = context->app_data;

    if(dm_timer_elapsed(&app_data->frame_timer, context) >= 1)
    {
        DM_LOG_INFO("FPS: %u", app_data->frame_count);
        app_data->frame_count = 0;
        dm_timer_start(&app_data->frame_timer, context);
    }
    else
    {
        app_data->frame_count++;
    }

    return true;
}

bool dm_application_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    dm_render_command_bind_raster_pipeline(app_data->raster_pipe, context);
    dm_render_command_bind_vertex_buffer(app_data->vb, context);
    dm_render_command_draw_instanced(1,0,_countof(triangle),0, context);

    return true;
}
