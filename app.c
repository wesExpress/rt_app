#include "dm.h"
#include "data.h"

typedef struct gui_vertex_t
{
    float pos[2];
    float uv[2];
    float color[4];
} gui_vertex;

#define MAX_GUI_VERTICES 100

typedef struct simple_camera_t
{
    dm_vec3 pos, forward, up;
    dm_mat4 view, proj;
} simple_camera;

typedef struct application_data_t
{
    dm_render_handle raster_pipe;
    dm_render_handle vb, ib, cb;

    dm_render_handle gui_vb, gui_cb;
    dm_render_handle gui_pipe;
    dm_font          gui_font;

    gui_vertex gui_vertices[MAX_GUI_VERTICES];
    uint32_t   gui_vertex_count;

    simple_camera camera;
    dm_mat4       model;
    dm_mat4       mvp;
    dm_vec3       axis;
    float         angle;

    dm_mat4 gui_proj;

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
        desc.descriptor_group[0].ranges[0].count      = 1;
        desc.descriptor_group[0].flags                = DM_DESCRIPTOR_GROUP_FLAG_VERTEX_SHADER;

        desc.descriptor_group[0].range_count = 1;

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
        if(!dm_renderer_load_font("assets/JetBrainsMono-Regular.ttf", 16, &app_data->gui_font, context)) return false;

        dm_vertex_buffer_desc vb_desc = { 0 };
        vb_desc.stride       = sizeof(gui_vertex);
        vb_desc.element_size = sizeof(float);
        vb_desc.size         = sizeof(gui_vertex) * MAX_GUI_VERTICES;
        vb_desc.data         = NULL;

        if(!dm_renderer_create_vertex_buffer(vb_desc, &app_data->gui_vb, context)) return false;

        dm_mat_ortho(0,(float)context->renderer.width, (float)context->renderer.height,0, -1,1, app_data->gui_proj);
#ifdef DM_DIRECTX12
        dm_mat4_transpose(app_data->gui_proj, app_data->gui_proj);
#endif

        dm_constant_buffer_desc cb_desc = { 0 };
        cb_desc.size = sizeof(dm_mat4);
        cb_desc.data = app_data->gui_proj;

        if(!dm_renderer_create_constant_buffer(cb_desc, &app_data->gui_cb, context)) return false;

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

        // descriptors
        desc.descriptor_group[0].ranges[0].type       = DM_DESCRIPTOR_RANGE_TYPE_CONSTANT_BUFFER;
        desc.descriptor_group[0].ranges[0].count      = 1;
        desc.descriptor_group[0].flags                = DM_DESCRIPTOR_GROUP_FLAG_VERTEX_SHADER;

        desc.descriptor_group[0].range_count = 1;

        desc.descriptor_group[1].ranges[0].type       = DM_DESCRIPTOR_RANGE_TYPE_TEXTURE;
        desc.descriptor_group[1].ranges[0].count      = 1;
        desc.descriptor_group[1].flags                = DM_DESCRIPTOR_GROUP_FLAG_PIXEL_SHADER;

        desc.descriptor_group[1].range_count = 1;

        desc.descriptor_group_count = 2;

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

    // gui
    const float width  = 400.f;
    const float height = 700.f;

    // test rect

    app_data->gui_vertices[0].pos[0]   = 100.f;
    app_data->gui_vertices[0].pos[1]   = 100.f;
    app_data->gui_vertices[0].color[0] = 1.f;
    app_data->gui_vertices[0].color[3] = 1.f;

    app_data->gui_vertices[1].pos[0]   = 100.f;
    app_data->gui_vertices[1].pos[1]   = 100.f + height;
    app_data->gui_vertices[1].color[0] = 1.f;
    app_data->gui_vertices[1].color[3] = 1.f;

    app_data->gui_vertices[2].pos[0]   = 100.f + width;
    app_data->gui_vertices[2].pos[1]   = 100.f;
    app_data->gui_vertices[2].color[0] = 1.f;
    app_data->gui_vertices[2].color[3] = 1.f;

    app_data->gui_vertices[3].pos[0]   = 100.f + width;
    app_data->gui_vertices[3].pos[1]   = 100.f;
    app_data->gui_vertices[3].color[0] = 1.f;
    app_data->gui_vertices[3].color[3] = 1.f;

    app_data->gui_vertices[4].pos[0]   = 100.f;
    app_data->gui_vertices[4].pos[1]   = 100.f + height;
    app_data->gui_vertices[4].color[0] = 1.f;
    app_data->gui_vertices[4].color[3] = 1.f;

    app_data->gui_vertices[5].pos[0]   = 100.f + width;
    app_data->gui_vertices[5].pos[1]   = 100.f + height;
    app_data->gui_vertices[5].color[0] = 1.f;
    app_data->gui_vertices[5].color[3] = 1.f;

    app_data->gui_vertex_count = 6;

    return true;
}

bool dm_application_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    // object rendering
    dm_render_command_update_constant_buffer(app_data->mvp, sizeof(dm_mat4), app_data->cb, context);

    dm_render_command_bind_raster_pipeline(app_data->raster_pipe, context);
    dm_render_command_bind_constant_buffer(app_data->cb, 0, context);
    dm_render_command_bind_descriptor_group(app_data->raster_pipe, 0, context);

    dm_render_command_bind_vertex_buffer(app_data->vb, context);
    dm_render_command_bind_index_buffer(app_data->ib, context);

    dm_render_command_draw_instanced_indexed(1,0, _countof(cube_indices),0, 0, context);

    // gui rendering
    dm_render_command_update_vertex_buffer(app_data->gui_vertices, sizeof(app_data->gui_vertices), app_data->gui_vb, context);

    dm_render_command_bind_raster_pipeline(app_data->gui_pipe, context);
    dm_render_command_bind_constant_buffer(app_data->gui_cb, 0, context);
    dm_render_command_bind_texture(app_data->gui_font.texture_handle, 0, context);
    dm_render_command_bind_descriptor_group(app_data->gui_pipe, 0, context);

    dm_render_command_bind_vertex_buffer(app_data->gui_vb, context);

    dm_render_command_draw_instanced(1,0, app_data->gui_vertex_count,0, context);

    app_data->gui_vertex_count = 0;

    return true;
}
