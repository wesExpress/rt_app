#include "dm.h"

typedef struct vertex_t
{
    float pos[3];
    float color[4];
} vertex;

const vertex triangle[] = {
    { { -0.5f,-0.5f,0.f }, { 1.f,0.f,0.f,1.f } },
    { {  0.5f,-0.5f,0.f }, { 0.f,1.f,0.f,1.f } },
    { {  0.f,  0.5f,0.f }, { 0.f,0.f,1.f,1.f } }
};

const vertex quad[] = {
    { { -0.5f,-0.5f,0.f }, { 1.f,0.f,0.f,1.f } },
    { {  0.5f,-0.5f,0.f }, { 0.f,1.f,0.f,1.f } },
    { { -0.5f, 0.5f,0.f }, { 0.f,0.f,1.f,1.f } },
    { {  0.5f, 0.5f,0.f }, { 1.f,1.f,0.f,1.f } }
};

const uint32_t quad_indices[] = { 0,1,2, 3,2,1 };

typedef struct simple_camera_t
{
    dm_vec3 pos, forward, up;
    dm_mat4 view, proj;
} simple_camera;

typedef struct camera_uniform_data_t 
{
    dm_mat4 mvp;
} camera_uniform_data;

typedef struct application_data_t
{
    dm_render_handle raster_pipe;
    dm_render_handle vb, ib, cb;

    simple_camera       camera;
    camera_uniform_data mvp;

    dm_timer frame_timer;
    uint16_t frame_count;
} application_data;

void dm_application_setup(dm_context_init_packet* init_packet)
{
    init_packet->app_data_size = sizeof(application_data);
}

bool dm_application_init(dm_context* context)
{
    application_data* app_data = context->app_data;

    // === camera ===
    {
        app_data->camera.pos[2] = -3.f;
        app_data->camera.up[1]  = 1.f;
        dm_vec3_negate(app_data->camera.pos, app_data->camera.forward);
        
        dm_mat_view(app_data->camera.pos, app_data->camera.forward, app_data->camera.up, app_data->camera.view);
        dm_mat_perspective(65.f, (float)context->renderer.width / (float)context->renderer.height, 0.01f, 1000.f, app_data->camera.proj);

        dm_mat4_mul_mat4(app_data->camera.view, app_data->camera.proj, app_data->mvp.mvp);
    }

    // === vertex buffer ===
    {
        dm_vertex_buffer_desc desc = { 0 };
        //desc.size         = sizeof(triangle);
        desc.size         = sizeof(quad);
        desc.element_size = sizeof(float);
        desc.stride       = sizeof(vertex);
        //desc.data         = (void*)triangle;
        desc.data         = (void*)quad;

        if(!dm_renderer_create_vertex_buffer(desc, &app_data->vb, context)) return false;
    }

    // === index buffer ===
    {
        dm_index_buffer_desc desc = { 0 };
        desc.size         = sizeof(quad_indices);
        desc.element_size = sizeof(uint32_t);
        desc.data         = (void*)quad_indices;
        desc.index_type   = DM_INDEX_BUFFER_INDEX_TYPE_UINT32;

        if(!dm_renderer_create_index_buffer(desc, &app_data->ib, context)) return false;
    }

    // === constant buffer ===
    {
        dm_constant_buffer_desc desc = { 0 };
        desc.size = sizeof(camera_uniform_data);
        desc.data = app_data->mvp.mvp;

        if(!dm_renderer_create_constant_buffer(desc, &app_data->cb, context)) return false;
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

        dm_strcpy(input->name, "COLOR");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_4;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(vertex);
        input->offset = offsetof(vertex, color);

        desc.input_assembler.topology = DM_INPUT_TOPOLOGY_TRIANGLE_LIST;

        desc.input_assembler.input_element_count = 2;

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

    // camera buffer 
    if(dm_input_is_key_pressed(DM_KEY_W, context))
    {
        app_data->camera.pos[2] += 1.f * context->delta;
    }
    else if(dm_input_is_key_pressed(DM_KEY_S, context))
    {
        app_data->camera.pos[2] -= 1.f * context->delta;
    }

    if(dm_input_is_key_pressed(DM_KEY_A, context))
    {
        app_data->camera.pos[0] -= 1.f * context->delta;
    }
    else if(dm_input_is_key_pressed(DM_KEY_D, context))
    {
        app_data->camera.pos[0] += 1.f * context->delta;
    }

    dm_mat_view(app_data->camera.pos, app_data->camera.forward, app_data->camera.up, app_data->camera.view);
    dm_mat4_mul_mat4(app_data->camera.view, app_data->camera.proj, app_data->mvp.mvp);

    return true;
}

bool dm_application_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    dm_render_command_update_constant_buffer(app_data->mvp.mvp, sizeof(dm_mat4), app_data->cb, context);

    dm_render_command_bind_raster_pipeline(app_data->raster_pipe, context);
    dm_render_command_bind_vertex_buffer(app_data->vb, context);
    dm_render_command_bind_index_buffer(app_data->ib, context);
    //dm_render_command_draw_instanced(1,0,_countof(quad),0, context);
    dm_render_command_draw_instanced_indexed(1,0, 6,0, 0, context);

    return true;
}
