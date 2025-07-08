#include "app.h"
#include "rendering/gui.h"
#include "rendering/debug_pipeline.h"

#include <stdio.h>

void dm_application_setup(dm_context_init_packet* init_packet)
{
    init_packet->app_data_size = sizeof(application_data);
}

bool dm_application_init(dm_context* context)
{
    application_data* app_data = context->app_data;

    // === camera ===
    dm_vec3 pos = { 0,0,6 };
    dm_vec3 forward = { 0,0,-1 };

    camera_init(pos, forward, &app_data->camera, context);

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

        if(!gui_load_font("assets/fonts/JetBrainsMono-Regular.ttf", 16, &app_data->font16, app_data->gui_context, context)) return false;
        if(!gui_load_font("assets/fonts/JetBrainsMono-Regular.ttf", 32, &app_data->font32, app_data->gui_context, context)) return false;
    }

    if(!raster_pipeline_init(context)) return false;
    if(!init_entities(context)) return false;
    //if(!rt_pipeline_init(context)) return false;
    if(!init_entity_pipeline(context)) return false;
    if(!debug_pipeline_init(context)) return false;

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

    // camera 
    camera_update(&app_data->camera, context);

    // various updates
    if(!raster_pipeline_update(context)) return false;
    //if(!rt_pipeline_update(context))     return false;
    if(!debug_pipeline_update(context))  return false;
    
    // gui
    static float quad_color[] = { 0.1f,0.1f,0.7f,1.f };
    static float quad_border_color[] = { 0.f,0.f,0.f,1.f };

    gui_draw_quad_border(100.f,100.f, 500.f,200.f, quad_color, quad_border_color, app_data->gui_context);

    static const float fps_color[] = { 1.f,1.f,1.f,1.f };
    static const float frame_timer_color[] = { 1.f,1.f,1.f,1.f };

    char t[512];
    sprintf(t, "Instance count: %u", MAX_ENTITIES);

    gui_draw_text(110.f,110.f, app_data->fps_text,        fps_color, app_data->font16, app_data->gui_context);
    gui_draw_text(110.f,160.f, app_data->frame_time_text, frame_timer_color, app_data->font32, app_data->gui_context);
    gui_draw_text(110.f,210.f, t, fps_color, app_data->font16, app_data->gui_context); 

    sprintf(t, "Camera pos: %f %f %f", app_data->camera.pos[0], app_data->camera.pos[1], app_data->camera.pos[2]);
    gui_draw_text(110.f,250.f, t, fps_color, app_data->font16, app_data->gui_context);
    sprintf(t, "Camera forward: %f %f %f", app_data->camera.forward[0], app_data->camera.forward[1], app_data->camera.forward[2]);
    gui_draw_text(110.f,270.f, t, fps_color, app_data->font16, app_data->gui_context);

    if(dm_input_key_just_pressed(DM_KEY_SPACE, context)) app_data->ray_trace = !app_data->ray_trace;

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

    return true;
}

bool dm_application_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    update_entities(context);

    gui_update_buffers(app_data->gui_context, context);

    // render after
    dm_render_command_begin_render_pass(0.2f,0.5f,0.7f,1.f, context);

    if(app_data->ray_trace)
    {
        //if(!rt_pipeline_render(context)) return false;
    }
    else 
    {
        if(!raster_pipeline_render(context)) return false;
    }

    // gui 
    gui_render(app_data->gui_context, context);

    debug_pipeline_render(context);

    // 
    dm_render_command_end_render_pass(context);

    return true;
}
