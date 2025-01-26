#include "dm.h"

typedef struct vertex_t
{
    float pos[3];
    float color[4];
} vertex;

typedef struct gui_vertex_t
{
    float pos[2];
    float uv[2];
    float color[4];
} gui_vertex;

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

const vertex cube[] = {
    // front face
    { { -0.5f,-0.5f,0.5f }, {1.0f,0.f,0.f,1.f } },  // 0
    { {  0.5f,-0.5f,0.5f }, {1.0f,0.f,0.f,1.f } },  // 1
    { { -0.5f, 0.5f,0.5f }, {1.0f,0.f,0.f,1.f } },  // 2
    { {  0.5f, 0.5f,0.5f }, {1.0f,0.f,0.f,1.f } },  // 3

    // back face
    { { -0.5f,-0.5f,-0.5f }, {0.0f,1.f,0.f,1.f } }, // 4
    { {  0.5f,-0.5f,-0.5f }, {0.0f,1.f,0.f,1.f } }, // 5
    { { -0.5f, 0.5f,-0.5f }, {0.0f,1.f,0.f,1.f } }, // 6
    { {  0.5f, 0.5f,-0.5f }, {0.0f,1.f,0.f,1.f } }, // 7
    
    // left face
    { { -0.5f,-0.5f,-0.5f }, {0.0f,0.f,1.f,1.f } },  // 8
    { { -0.5f,-0.5f, 0.5f }, {0.0f,0.f,1.f,1.f } },  // 9
    { { -0.5f, 0.5f, 0.5f }, {0.0f,0.f,1.f,1.f } },  // 10
    { { -0.5f, 0.5f,-0.5f }, {0.0f,0.f,1.f,1.f } },  // 11

    // right face
    { {  0.5f,-0.5f,-0.5f }, {1.0f,1.f,0.f,1.f } },  // 12 
    { {  0.5f,-0.5f, 0.5f }, {1.0f,1.f,0.f,1.f } },  // 13
    { {  0.5f, 0.5f, 0.5f }, {1.0f,1.f,0.f,1.f } },  // 14
    { {  0.5f, 0.5f,-0.5f }, {1.0f,1.f,0.f,1.f } },  // 15
    
    // top face
    { { -0.5f, 0.5f, 0.5f }, {1.0f,0.f,1.f,1.f } },  // 16 
    { {  0.5f, 0.5f, 0.5f }, {1.0f,0.f,1.f,1.f } },  // 17
    { { -0.5f, 0.5f,-0.5f }, {1.0f,0.f,1.f,1.f } },  // 18
    { {  0.5f, 0.5f,-0.5f }, {1.0f,0.f,1.f,1.f } },  // 19

    // bottom face
    { { -0.5f,-0.5f, 0.5f }, {0.0f,1.f,1.f,1.f } },  // 20 
    { {  0.5f,-0.5f, 0.5f }, {0.0f,1.f,1.f,1.f } },  // 21 
    { { -0.5f,-0.5f,-0.5f }, {0.0f,1.f,1.f,1.f } },  // 22 
    { {  0.5f,-0.5f,-0.5f }, {0.0f,1.f,1.f,1.f } },  // 23 
};

const uint32_t cube_indices[] = {
    0,1,2, 3,2,1,

    4,6,5, 7,5,6,

    8,9,10, 10,11,8,

    12,15,14, 14,13,12,

    16,17,18, 18,17,19,

    22,23,21, 21,20,22 
};

#define MAX_GUI_VERTICES 100
gui_vertex gui_vertices[MAX_GUI_VERTICES] = { 0 };

typedef struct simple_camera_t
{
    dm_vec3 pos, forward, up;
    dm_mat4 view, proj;
} simple_camera;

typedef struct application_data_t
{
    dm_render_handle raster_pipe;
    dm_render_handle vb, ib, cb;

    dm_render_handle gui_vb;
    dm_render_handle gui_pipe;

    simple_camera camera;
    dm_mat4       model;
    dm_mat4       mvp;
    dm_vec3       axis;
    float         angle;

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
        app_data->camera.pos[2] = 3.f;
        app_data->camera.up[1]  = 1.f;
        dm_vec3_negate(app_data->camera.pos, app_data->camera.forward);
        
        dm_mat_view(app_data->camera.pos, app_data->camera.forward, app_data->camera.up, app_data->camera.view);
        dm_mat_perspective(DM_MATH_DEG_TO_RAD * 85.f, (float)context->renderer.width / (float)context->renderer.height, 0.1f, 1000.f, app_data->camera.proj);

        dm_mat4_mul_mat4(app_data->camera.view, app_data->camera.proj, app_data->mvp);
    }

    // === vertex buffer ===
    {
        dm_vertex_buffer_desc desc = { 0 };
        desc.size         = sizeof(cube);
        desc.element_size = sizeof(float);
        desc.stride       = sizeof(vertex);
        desc.data         = (void*)cube;

        if(!dm_renderer_create_vertex_buffer(desc, &app_data->vb, context)) return false;
    }

    // === index buffer ===
    {
        dm_index_buffer_desc desc = { 0 };
        desc.size         = sizeof(cube_indices);
        desc.element_size = sizeof(uint32_t);
        desc.data         = (void*)cube_indices;
        desc.index_type   = DM_INDEX_BUFFER_INDEX_TYPE_UINT32;

        if(!dm_renderer_create_index_buffer(desc, &app_data->ib, context)) return false;
    }

    // === constant buffer ===
    {
        dm_constant_buffer_desc desc = { 0 };
        desc.size = sizeof(dm_mat4);
        desc.data = app_data->mvp;

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

        // descriptor groups
        desc.descriptor_group[0].ranges[0].type       = DM_DESCRIPTOR_RANGE_TYPE_CONSTANT_BUFFER;
        desc.descriptor_group[0].ranges[0].flags      = DM_DESCRIPTOR_FLAG_VERTEX_SHADER;
        desc.descriptor_group[0].ranges[0].count      = 1;
        desc.descriptor_group[0].ranges[0].handles[0] = app_data->cb;

        desc.descriptor_group[0].range_count     = 1;

        desc.descriptor_group_count = 1;

        if(!dm_renderer_create_raster_pipeline(desc, &app_data->raster_pipe, context)) return false;
    }


    // misc
    dm_timer_start(&app_data->frame_timer, context);

    dm_mat4_identity(app_data->model);
    app_data->axis[0] = 1.f;
    app_data->axis[1] = 1.f;
    app_data->axis[2] = 1.f;
    dm_vec3_norm(app_data->axis, app_data->axis);

    // gui
    {
        dm_raster_pipeline_desc desc = { 0 };

        dm_input_element_desc* input = desc.input_assembler.input_elements;
        dm_strcpy(input->name, "POSITION");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_2;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(gui_vertex);
        input->offset = offsetof(gui_vertex, pos);

        input++;

        dm_strcpy(input->name, "TEX_COORDS");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_2;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(gui_vertex);
        input->offset = offsetof(gui_vertex, pos);

        input++;

        dm_strcpy(input->name, "COLOR");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_4;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(gui_vertex);
        input->offset = offsetof(gui_vertex, color);

        desc.input_assembler.topology = DM_INPUT_TOPOLOGY_TRIANGLE_LIST;

        desc.input_assembler.input_element_count = 3;

        // rasterizer
        desc.rasterizer.cull_mode    = DM_RASTERIZER_CULL_MODE_BACK;
        desc.rasterizer.polygon_fill = DM_RASTERIZER_POLYGON_FILL_FILL;
        desc.rasterizer.front_face   = DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE;
        dm_strcpy(desc.rasterizer.vertex_shader_desc.path, "assets/gui_vertex.cso");
        dm_strcpy(desc.rasterizer.pixel_shader_desc.path,  "assets/gui_pixel.cso");

        desc.viewport.type = DM_VIEWPORT_TYPE_DEFAULT;
        desc.scissor.type  = DM_SCISSOR_TYPE_DEFAULT;

        if(!dm_renderer_create_raster_pipeline(desc, &app_data->gui_pipe, context)) return false;
    }

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
    dm_vec3 move = { 0 };

    // z direction (into screen is negative)
    if(dm_input_is_key_pressed(DM_KEY_W, context))
    {
        move[2] = -1.f;
    }
    else if(dm_input_is_key_pressed(DM_KEY_S, context))
    {
        move[2] = 1.f;
    }

    // x direction
    if(dm_input_is_key_pressed(DM_KEY_A, context))
    {
        move[0] = -1.f;
    }
    else if(dm_input_is_key_pressed(DM_KEY_D, context))
    {
        move[0] = 1.f;
    }

    dm_vec3_norm(move, move);
    dm_vec3_scale(move, 5.f * context->delta, move);
    if(dm_vec3_mag2(move)>0) dm_vec3_add_vec3(app_data->camera.pos, move, app_data->camera.pos);

    dm_vec3 target = { 0 };
    dm_vec3_add_vec3(app_data->camera.pos, app_data->camera.forward, target);
    dm_mat_view(app_data->camera.pos, target, app_data->camera.up, app_data->camera.view);

    // model matrix
    app_data->angle += 45.f * context->delta;
    if(app_data->angle > 365.f) app_data->angle -= 365.f;

    dm_quat orientation;
    dm_quat_from_axis_angle_deg(app_data->axis, app_data->angle, orientation);

    dm_mat4 rotate;
    dm_mat4_rotate_from_quat(orientation, rotate);

    dm_mat4 mvp;
    dm_mat4_mul_mat4(app_data->model, rotate, mvp);
    dm_mat4_mul_mat4(mvp, app_data->camera.view, mvp);
    dm_mat4_mul_mat4(mvp, app_data->camera.proj, app_data->mvp);
#ifdef DM_DIRECTX12
    dm_mat4_transpose(app_data->mvp, app_data->mvp);
#endif

    return true;
}

bool dm_application_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    // object rendering
    dm_render_command_update_constant_buffer(app_data->mvp, sizeof(dm_mat4), app_data->cb, context);

    dm_render_command_bind_raster_pipeline(app_data->raster_pipe, context);
    dm_render_command_bind_descriptor_group(app_data->raster_pipe, 0, context);

    dm_render_command_bind_vertex_buffer(app_data->vb, context);
    dm_render_command_bind_index_buffer(app_data->ib, context);

    dm_render_command_draw_instanced_indexed(1,0, _countof(cube_indices),0, 0, context);

    // gui rendering
    dm_render_command_bind_raster_pipeline(app_data->gui_pipe, context);

    return true;
}
