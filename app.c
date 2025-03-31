#include "dm.h"
#include "data.h"
#include "rendering/gui.h"

#include <stdio.h>

#define MAX_INSTANCES 10000
#define INDEX_TYPE DM_INDEX_BUFFER_INDEX_TYPE_UINT32
#define CUBE_BLAS 0
#define WORLD_SCALE 100.f

typedef struct simple_camera_t
{
    dm_vec3 pos, forward, up;
    dm_mat4 view, proj;
} simple_camera;

typedef struct transform_t
{
    dm_vec4 position;
    dm_vec4 scale;
    dm_quat orientation;
} transform;

typedef struct instance_t 
{
    dm_mat4 model;
    dm_mat4 normal;
} instance;

typedef struct application_data_t
{
    dm_resource_handle raster_pipe;
    dm_resource_handle vb, ib, cb;

    dm_resource_handle transform_sb, model_sb;
    dm_resource_handle compute_pipe;

    dm_resource_handle rt_pipe, acceleration_structure;
    uint8_t            instance_blas[MAX_INSTANCES];
    size_t             blas_addresses[DM_TLAS_MAX_BLAS];

    dm_raytracing_instance raytracing_instances[MAX_INSTANCES];

    void* gui_context;
    uint8_t font16, font32;

    char       fps_text[512];
    char       frame_time_text[512];

    transform transforms[MAX_INSTANCES];

    dm_vec3 positions[MAX_INSTANCES];
    dm_vec3 scales[MAX_INSTANCES];
    dm_vec3 axes[MAX_INSTANCES];
    float   angles[MAX_INSTANCES];

    instance instances[MAX_INSTANCES];

    simple_camera camera;
    dm_mat4       model;
    dm_mat4       vp;
    dm_vec3       axis;
    float         angle;

    dm_mat4 gui_proj;

    dm_timer frame_timer, fps_timer;
    uint16_t frame_count;
} application_data;

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
        desc.index_type   = INDEX_TYPE;

        if(!dm_renderer_create_index_buffer(desc, &app_data->ib, context)) return false;
    }

    // === constant buffer ===
    {
        dm_constant_buffer_desc desc = { 0 };
        desc.size = sizeof(dm_mat4);
        desc.data = app_data->vp;

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

        if(!dm_renderer_create_raster_pipeline(desc, &app_data->raster_pipe, context)) return false;
    }

    // === raytracing ===
    {
        dm_acceleration_structure_desc desc = { 0 };
        desc.tlas.blas[0].flags         = DM_BLAS_GEOMETRY_FLAG_OPAQUE;
        desc.tlas.blas[0].geometry_type = DM_BLAS_GEOMETRY_TYPE_TRIANGLES;
        desc.tlas.blas[0].vertex_type   = DM_BLAS_VERTEX_TYPE_FLOAT_3;
        desc.tlas.blas[0].vertex_count  = sizeof(cube) / sizeof(vertex); 
        desc.tlas.blas[0].vertex_buffer = app_data->vb;
        desc.tlas.blas[0].vertex_stride = sizeof(vertex);
        desc.tlas.blas[0].index_type    = INDEX_TYPE;
        desc.tlas.blas[0].index_count   = _countof(cube_indices);
        desc.tlas.blas[0].index_buffer  = app_data->ib;

        desc.tlas.blas_count     = 1;
        desc.tlas.instance_count = MAX_INSTANCES;
        desc.tlas.instances = app_data->raytracing_instances;
        
        if(!dm_renderer_create_acceleration_structure(desc, &app_data->acceleration_structure, context)) return false;

        dm_raytracing_pipeline_desc rt_pipe_desc = { 0 };

        rt_pipe_desc.hit_group_count = 1;
        rt_pipe_desc.hit_groups[0].type = DM_RT_PIPE_HIT_GROUP_TYPE_TRIANGLES;
        dm_strcpy(rt_pipe_desc.hit_groups[0].name, "hit_group");
        dm_strcpy(rt_pipe_desc.hit_groups[0].shaders[DM_RT_PIPE_HIT_GROUP_STAGE_CLOSEST], "closest_hit");
        dm_strcpy(rt_pipe_desc.hit_groups[0].path, "assets/rt_shader.cso");
        rt_pipe_desc.hit_groups[0].flags |= DM_RT_PIPE_HIT_GROUP_FLAG_CLOSEST;

        rt_pipe_desc.global_descriptor_groups[0].descriptors[0] = DM_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;
        rt_pipe_desc.global_descriptor_groups[0].descriptors[1] = DM_DESCRIPTOR_TYPE_WRITE_TEXTURE;
        rt_pipe_desc.global_descriptor_groups[0].descriptors[2] = DM_DESCRIPTOR_TYPE_CONSTANT_BUFFER;
        rt_pipe_desc.global_descriptor_groups[0].count          = 3;
        rt_pipe_desc.global_descriptor_group_count              = 1;

        //if(!dm_renderer_create_raytracing_pipeline(rt_pipe_desc, &app_data->rt_pipe, context)) return false;
    }

    // compute pipeline
    {
        dm_storage_buffer_desc b_desc = { 0 };
        b_desc.write        = true;
        b_desc.size         = MAX_INSTANCES * sizeof(transform);
        b_desc.stride       = sizeof(transform);
        b_desc.data         = app_data->transforms;

        if(!dm_renderer_create_storage_buffer(b_desc, &app_data->transform_sb, context)) return false;

        b_desc.size         = MAX_INSTANCES * sizeof(instance);
        b_desc.stride       = sizeof(instance);
        b_desc.data         = NULL;
        if(!dm_renderer_create_storage_buffer(b_desc, &app_data->model_sb, context)) return false;

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

        if(!dm_compute_create_compute_pipeline(desc, &app_data->compute_pipe, context)) return false;
    }

    // misc
    dm_timer_start(&app_data->frame_timer, context);
    dm_timer_start(&app_data->fps_timer, context);

    dm_mat4_identity(app_data->model);
    app_data->axis[0] = 1.f;
    app_data->axis[1] = 1.f;
    app_data->axis[2] = 1.f;
    dm_vec3_norm(app_data->axis, app_data->axis);

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
    if(!dm_renderer_get_blas_gpu_address(app_data->acceleration_structure, CUBE_BLAS, &app_data->blas_addresses[CUBE_BLAS], context)) return false;

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

    // update the transforms and compute model matrices
    dm_compute_command_bind_compute_pipeline(app_data->compute_pipe, context);
    dm_compute_command_bind_storage_buffer(app_data->transform_sb, 0,0, context);
    dm_compute_command_bind_storage_buffer(app_data->model_sb, 1,0, context);
    dm_compute_command_bind_descriptor_group(0,2,0, context);
    dm_compute_command_dispatch(1024,1,1, context);

    dm_render_command_update_acceleration_structure(app_data->raytracing_instances, sizeof(app_data->raytracing_instances), MAX_INSTANCES, app_data->acceleration_structure, context);

    // render after
    dm_render_command_begin_render_pass(0.2f,0.5f,0.7f,1.f, context);

    // object rendering
    dm_render_command_update_constant_buffer(app_data->vp, sizeof(dm_mat4), app_data->cb, context);

    dm_render_command_bind_raster_pipeline(app_data->raster_pipe, context);
    dm_render_command_bind_constant_buffer(app_data->cb, 0,0, context);
    dm_render_command_bind_storage_buffer(app_data->model_sb, 1,0, context);
    dm_render_command_bind_descriptor_group(0,2,0, context);

    dm_render_command_bind_vertex_buffer(app_data->vb, 0, context);
    dm_render_command_bind_index_buffer(app_data->ib, context);

    dm_render_command_draw_instanced_indexed(MAX_INSTANCES,0, _countof(cube_indices),0, 0, context);

    // gui 
    gui_render(app_data->gui_context, context);

    // 
    dm_render_command_end_render_pass(context);

    return true;
}
