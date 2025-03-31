#include "app.h"
#include "rendering/gui.h"

#include <stdio.h>

#define WORLD_SCALE 100.f

void dm_application_setup(dm_context_init_packet* init_packet)
{
    init_packet->app_data_size = sizeof(application_data);
}

#ifndef DM_DEBUG
DM_INLINE
#endif
transform init_transform(float world_scale, dm_context* context)
{
    transform t;

    t.position[0] = dm_random_float(context) * world_scale - world_scale * 0.5f;
    t.position[1] = dm_random_float(context) * world_scale - world_scale * 0.5f;
    t.position[2] = dm_random_float(context) * world_scale - world_scale * 0.5f;

    t.scale[0] = dm_random_float(context);
    t.scale[1] = dm_random_float(context);
    t.scale[2] = dm_random_float(context);

    t.orientation[0] = dm_random_float(context);
    t.orientation[1] = dm_random_float(context);
    t.orientation[2] = dm_random_float(context);
    t.orientation[3] = 1.f;
    dm_quat_norm(t.orientation, t.orientation);

    return t;
}

bool dm_application_init(dm_context* context)
{
    application_data* app_data = context->app_data;

    // initialize instances
    for(uint32_t i=0; i<MAX_INSTANCES; i++)
    {
        app_data->transforms[i] = init_transform(WORLD_SCALE, context);

#if 0
        dm_mat4_identity(app_data->instances[i].model);
        dm_mat4 rotation;
        dm_quat orient;
        dm_quat_from_axis_angle_deg(app_data->axes[i], app_data->angles[i], orient);
        dm_mat4_rotate_from_quat(orient, rotation);

        dm_mat_translate(app_data->instances[i].model, app_data->positions[i], app_data->instances[i].model);
        dm_mat4_mul_mat4(app_data->instances[i].model, rotation, app_data->instances[i].model);

        dm_mat_scale(app_data->instances[i].model, app_data->scales[i], app_data->instances[i].model);
#ifdef DM_DIRECTX12
        dm_mat4_transpose(app_data->instances[i].model, app_data->instances[i].model);
#endif
#endif
        // raytracing instances
        //dm_memcpy(app_data->raytracing_instances[i].transform, app_data->instances[i].obj_model, sizeof(float) * 3 * 4);
        app_data->raytracing_instances[i].blas_address = 0;
        app_data->raytracing_instances[i].id = i;
    }

    // === camera ===
    {
        app_data->camera.pos[2] = 3.f;
        app_data->camera.up[1]  = 1.f;
        dm_vec3_negate(app_data->camera.pos, app_data->camera.forward);
        
        dm_mat_view(app_data->camera.pos, app_data->camera.forward, app_data->camera.up, app_data->camera.view);
        dm_mat_perspective(DM_MATH_DEG_TO_RAD * 75.f, (float)context->renderer.width / (float)context->renderer.height, 0.1f, 1000.f, app_data->camera.proj);

        dm_mat4_mul_mat4(app_data->camera.view, app_data->camera.proj, app_data->vp);
    }

    if(!raster_pipeline_init(context)) return false;
    if(!rt_pipeline_init(app_data->raster_data.vb, app_data->raster_data.ib, context)) return false;

    // misc
    dm_timer_start(&app_data->frame_timer, context);
    dm_timer_start(&app_data->fps_timer, context);

    // gui
    {
        gui_style style = { 0 };
        style.text_padding_l = 15.f;
        style.text_padding_r = 15.f;
        style.text_padding_b = 15.f;
        style.text_padding_t = 15.f;

        style.window_border_h = 5.f;
        style.window_border_w = 5.f;

        style.window_border_color[3] = 1.f;

        if(!gui_init(style, 2, &app_data->gui_context, context)) 
        {
            DM_LOG_ERROR("Could not initialize gui rendering");
            return false;
        }

        if(!gui_load_font("assets/JetBrainsMono-Regular.ttf", 16, &app_data->font16, app_data->gui_context, context)) return false;
        if(!gui_load_font("assets/JetBrainsMono-Regular.ttf", 32, &app_data->font32, app_data->gui_context, context)) return false;
    }

    return true;
}

void dm_application_shutdown(dm_context* context)
{
    application_data* app_data = context->app_data;

    dm_free(&app_data->gui_context);
}

bool dm_application_update(dm_context* context)
{
    application_data* app_data = context->app_data;

    double frame_time = dm_timer_elapsed_ms(&app_data->frame_timer, context);
    sprintf(app_data->frame_time_text, "Frame time: %0.2lf ms", frame_time);
    dm_timer_start(&app_data->frame_timer, context);

    if(dm_timer_elapsed(&app_data->fps_timer, context) >= 1)
    {
        sprintf(app_data->fps_text, "FPS: %u", app_data->frame_count);
        app_data->frame_count = 0;
        dm_timer_start(&app_data->fps_timer, context);
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

    dm_mat4_mul_mat4(app_data->camera.view, app_data->camera.proj, app_data->vp);
#ifdef DM_DIRECTX12
    dm_mat4_transpose(app_data->vp, app_data->vp);
#endif

    // get blas gpu address

    // update models
#if 0
    for(uint32_t i=0; i<MAX_INSTANCES; i++)
    {
        app_data->angles[i] += 60.f * context->delta;
        if(app_data->angles[i] >= 360.f) app_data->angles[i] -= 360.f;

        dm_mat4 rotation;
        dm_quat orient;

        dm_quat_from_axis_angle_deg(app_data->axes[i], app_data->angles[i], orient);
        dm_mat4_rotate_from_quat(orient, rotation);

        dm_mat4_identity(app_data->instances[i].obj_model);
        dm_mat_scale(app_data->instances[i].obj_model, app_data->scales[i], app_data->instances[i].obj_model);
        dm_mat4_mul_mat4(app_data->instances[i].obj_model, rotation, app_data->instances[i].obj_model);
        dm_mat_translate(app_data->instances[i].obj_model, app_data->positions[i], app_data->instances[i].obj_model);
#ifdef DM_DIRECTX12
        dm_mat4_transpose(app_data->instances[i].obj_model, app_data->instances[i].obj_model);
#endif

        // raytracing instances
        dm_memcpy(app_data->raytracing_instances[i].transform, app_data->instances[i].obj_model, sizeof(float) * 3 * 4);
        app_data->raytracing_instances[i].blas_address = app_data->blas_addresses[CUBE_BLAS];
    }
#endif
    // gui
    static float quad_color[] = { 0.1f,0.1f,0.7f,1.f };
    static float quad_border_color[] = { 0.f,0.f,0.f,1.f };

    gui_draw_quad(100.f,100.f, 500.f,200.f, quad_color, app_data->gui_context);
    gui_draw_quad_border(100.f,500.f, 500.f,200.f, quad_color, quad_border_color, app_data->gui_context);

    static const float fps_color[] = { 1.f,1.f,1.f,1.f };
    static const float frame_timer_color[] = { 1.f,1.f,1.f,1.f };

    char t[512];
    sprintf(t, "Instance count: %u", MAX_INSTANCES);

    gui_draw_text(110.f,110.f, app_data->fps_text,        fps_color, app_data->font16, app_data->gui_context);
    gui_draw_text(110.f,160.f, app_data->frame_time_text, frame_timer_color, app_data->font32, app_data->gui_context);
    gui_draw_text(110.f,210.f, t, fps_color, app_data->font16, app_data->gui_context); 

    return true;
}

bool dm_application_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    gui_update_buffers(app_data->gui_context, context);


    // render after
    dm_render_command_begin_render_pass(0.2f,0.5f,0.7f,1.f, context);

    if(!raster_pipeline_render(context)) return false;

    // gui 
    gui_render(app_data->gui_context, context);

    // 
    dm_render_command_end_render_pass(context);

    return true;
}
