#include "app.h"
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
    dm_vec3 pos = { 0,5,0 };
    dm_vec3 forward = { -1,0,0 };

    camera_init(pos, forward, &app_data->camera, context);

    // misc
    dm_timer_start(&app_data->frame_timer, context);
    dm_timer_start(&app_data->fps_timer, context);

    // load in models
    // default textures 
    {
        dm_sampler_desc sampler_desc = {
            .address_u=DM_SAMPLER_ADDRESS_MODE_WRAP,
            .address_v=DM_SAMPLER_ADDRESS_MODE_WRAP,
            .address_w=DM_SAMPLER_ADDRESS_MODE_WRAP
        };

        if(!dm_renderer_create_sampler(sampler_desc, &app_data->default_sampler, context)) return false;

        uint32_t white[2][2] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
        uint32_t black[2][2] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };

        dm_texture_desc desc = {
            .format=DM_TEXTURE_FORMAT_BYTE_4_UNORM,
            .width=2,.height=2,.n_channels=4,
            .sampler=app_data->default_sampler,
            .data=white
        };

        if(!dm_renderer_create_texture(desc, &app_data->white_texture, context)) return false;

        desc.data = black;

        if(!dm_renderer_create_texture(desc, &app_data->black_texture, context)) return false;
    }

    dm_mesh_vertex_attribute attribs[] = {
        DM_MESH_VERTEX_ATTRIBUTE_POSITION_3_TEX_COORD_U,
        DM_MESH_VERTEX_ATTRIBUTE_NORMAL_3_TEX_COOR_V,
        DM_MESH_VERTEX_ATTRIBUTE_TANGENT_4,
        DM_MESH_VERTEX_ATTRIBUTE_COLOR_4
    };

    app_data->sponza_scene.white_texture   = app_data->white_texture;
    app_data->sponza_scene.black_texture   = app_data->black_texture;
    app_data->sponza_scene.default_sampler = app_data->default_sampler;

    //if(!dm_renderer_load_obj_model("assets/models/stanford_bunny.obj", attribs, _countof(attribs), &app_data->meshes[app_data->mesh_count++], context)) return false;
    //if(!dm_renderer_load_obj_model("assets/models/stanford_dragon.obj", attribs, _countof(attribs), &app_data->meshes[app_data->mesh_count++], context)) return false;
    //if(!LOAD_MESH_GLTF("assets/models/DamagedHelmet.glb", 0, attribs, &app_data->meshes[app_data->mesh_count++], &materials[app_data->material_count++], context)) return false;
    //if(!dm_renderer_load_gltf_file("assets/models/DamagedHelmet.glb", attribs, _countof(attribs), app_data->meshes, &app_data->mesh_count, materials, &app_data->material_count, context)) return false;
    if(!dm_renderer_load_gltf_file("assets/models/sponza-optimized/Sponza.gltf", attribs, _countof(attribs), &app_data->sponza_scene, context)) return false;

    {
        app_data->meshes = dm_alloc(sizeof(mesh) * app_data->sponza_scene.mesh_count);
        for(uint32_t i=0; i<app_data->sponza_scene.mesh_count; i++)
        {
            app_data->meshes[i].vb_index = app_data->sponza_scene.meshes[i].vb.descriptor_index;
            app_data->meshes[i].ib_index = app_data->sponza_scene.meshes[i].ib.descriptor_index;
        }

        dm_storage_buffer_desc desc = {
            .size=sizeof(mesh) * app_data->sponza_scene.mesh_count,
            .stride=sizeof(mesh),
            .data=app_data->meshes
        };

        if(!dm_renderer_create_storage_buffer(desc, &app_data->mesh_sb, context)) return false;
    }

    {
        dm_storage_buffer_desc desc = {
            .size=sizeof(dm_scene_node) * app_data->sponza_scene.node_count,
            .stride=sizeof(dm_scene_node),
            .data=app_data->sponza_scene.nodes
        };

        if(!dm_renderer_create_storage_buffer(desc, &app_data->node_buffer, context)) return false;
    }

    app_data->materials = dm_alloc(sizeof(material) * app_data->sponza_scene.material_count);

    for(uint8_t i=0; i<app_data->sponza_scene.material_count; i++)
    {
        app_data->materials[i].diffuse_texture_index    = app_data->sponza_scene.materials[i].textures[DM_MATERIAL_TYPE_DIFFUSE].descriptor_index + 1;
        app_data->materials[i].metallic_roughness_index = app_data->sponza_scene.materials[i].textures[DM_MATERIAL_TYPE_METALLIC_ROUGHNESS].descriptor_index + 1;
        app_data->materials[i].normal_map_index         = app_data->sponza_scene.materials[i].textures[DM_MATERIAL_TYPE_NORMAL_MAP].descriptor_index + 1;
        app_data->materials[i].specular_map_index       = app_data->sponza_scene.materials[i].textures[DM_MATERIAL_TYPE_SPECULAR_MAP].descriptor_index + 1;
        app_data->materials[i].occlusion_map_index      = app_data->sponza_scene.materials[i].textures[DM_MATERIAL_TYPE_OCCLUSION].descriptor_index + 1;
        app_data->materials[i].emissive_map_index       = app_data->sponza_scene.materials[i].textures[DM_MATERIAL_TYPE_EMISSION].descriptor_index + 1;

        app_data->materials[i].diffuse_sampler_index   = app_data->sponza_scene.materials[i].samplers[DM_MATERIAL_TYPE_DIFFUSE].descriptor_index;
        app_data->materials[i].metallic_sampler_index  = app_data->sponza_scene.materials[i].samplers[DM_MATERIAL_TYPE_METALLIC_ROUGHNESS].descriptor_index;
        app_data->materials[i].normal_sampler_index    = app_data->sponza_scene.materials[i].samplers[DM_MATERIAL_TYPE_NORMAL_MAP].descriptor_index;
        app_data->materials[i].specular_sampler_index  = app_data->sponza_scene.materials[i].samplers[DM_MATERIAL_TYPE_SPECULAR_MAP].descriptor_index;
        app_data->materials[i].occlusion_sampler_index = app_data->sponza_scene.materials[i].samplers[DM_MATERIAL_TYPE_OCCLUSION].descriptor_index;
        app_data->materials[i].emissive_sampler_index  = app_data->sponza_scene.materials[i].samplers[DM_MATERIAL_TYPE_EMISSION].descriptor_index;
    }
    app_data->material_count += app_data->sponza_scene.material_count;

    {
        dm_storage_buffer_desc desc = { 0 };
        desc.size   = sizeof(material) * app_data->material_count;
        desc.stride = sizeof(material);
        desc.data   = app_data->materials;

        if(!dm_renderer_create_storage_buffer(desc, &app_data->material_sb, context)) return false;

        app_data->lights[0].position_str[1] = 5.f;

        app_data->lights[0].color[0] = app_data->lights[0].color[1] = app_data->lights[0].color[2] = 1; 

        desc.size   = sizeof(app_data->lights);
        desc.stride = sizeof(light_source);
        desc.data   = &app_data->lights;

        if(!dm_renderer_create_storage_buffer(desc, &app_data->light_buffer, context)) return false;
    }

    dm_font_desc f16_desc = {
        .path="assets/fonts/JetBrainsMono-Regular.ttf",
        .size=16
    };

    dm_font_desc f32_desc = {
        .path="assets/fonts/JetBrainsMono-Regular.ttf",
        .size=32
    };
    dm_font_desc fonts[] = { f16_desc, f32_desc };
    if(!nuklear_gui_init(fonts, _countof(fonts), context)) return false;
    if(!raster_pipeline_init(context)) return false;
    if(!rt_pipeline_init(app_data->sponza_scene, context)) return false;
    if(!debug_pipeline_init(context)) return false;
    if(!quad_texture_init(context)) return false;

    // misc
    app_data->clear_color.a = 1;

    return true;
}

void dm_application_shutdown(dm_context* context)
{
    application_data* app_data = context->app_data;

    dm_free((void**)&app_data->materials);
    dm_free((void**)&app_data->sponza_scene.nodes);

    nk_font_atlas_clear(&app_data->nk_context.atlas);
    nk_buffer_clear(&app_data->nk_context.cmds);
    nk_free(&app_data->nk_context.ctx);
    dm_free(&app_data->nuklear_data);
}

bool dm_application_update(dm_context* context)
{
    application_data* app_data = context->app_data;

    char t[512];
    static const float fps_color[] = { 1.f,1.f,1.f,1.f };
    static const float frame_timer_color[] = { 1.f,1.f,1.f,1.f };

    double frame_time = dm_timer_elapsed_ms(&app_data->frame_timer, context);
    sprintf(app_data->frame_time_text, "Frame time: %0.2lf ms", frame_time);
    dm_timer_start(&app_data->frame_timer, context);

    // camera 
    camera_update(&app_data->camera, context);

    // gui
    nuklear_gui_update_input(context);

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

    if(nk_begin(&app_data->nk_context.ctx,"Application info", nk_rect(100,100, 350,550), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_DYNAMIC | NK_WINDOW_SCALABLE))
    {
        enum {EASY, HARD};
        static int op = EASY;
        static int property = 20;

        struct  nk_context* ctx = &app_data->nk_context.ctx;

        nk_style_set_font(ctx, &app_data->nk_context.fonts[1]->handle);
        nk_layout_row_dynamic(ctx, 30,1);
#ifdef DM_DIRECTX12
        nk_label(ctx, "Render backend: DX12", NK_TEXT_LEFT);
#elif defined(DM_VULKAN)
        nk_label(ctx, "Render backend: Vulkan", NK_TEXT_LEFT);
#endif

        nk_style_set_font(ctx, &app_data->nk_context.fonts[0]->handle);

        nk_layout_row_dynamic(ctx, 30,1);
        nk_label(ctx, app_data->fps_text, NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 30,1);
        nk_label(ctx, app_data->frame_time_text, NK_TEXT_LEFT);

        //nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "background:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_combo_begin_color(ctx, nk_rgb_cf(app_data->clear_color), nk_vec2(nk_widget_width(ctx),400))) {
            nk_layout_row_dynamic(ctx, 120, 1);
            app_data->clear_color = nk_color_picker(ctx, app_data->clear_color, NK_RGBA);
            nk_layout_row_dynamic(ctx, 25, 1);
            app_data->clear_color.r = nk_propertyf(ctx, "#R:", 0, app_data->clear_color.r, 1.0f, 0.01f,0.005f);
            app_data->clear_color.g = nk_propertyf(ctx, "#G:", 0, app_data->clear_color.g, 1.0f, 0.01f,0.005f);
            app_data->clear_color.b = nk_propertyf(ctx, "#B:", 0, app_data->clear_color.b, 1.0f, 0.01f,0.005f);
            app_data->clear_color.a = nk_propertyf(ctx, "#A:", 0, app_data->clear_color.a, 1.0f, 0.01f,0.005f);
            nk_combo_end(ctx);
        }

        // light source
        nk_label(ctx, "Light Position", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 4);
        app_data->lights[0].position_str[0] = nk_propertyf(ctx, "X", -100, app_data->lights[0].position_str[0], 100, 5.f,0.5f);
        app_data->lights[0].position_str[1] = nk_propertyf(ctx, "Y", -100, app_data->lights[0].position_str[1], 100, 5.f,0.5f);
        app_data->lights[0].position_str[2] = nk_propertyf(ctx, "Z", -100, app_data->lights[0].position_str[2], 100, 5.f,0.5f);
    }
    nk_end(&app_data->nk_context.ctx);

    app_data->rt_data.scene_data.sky_color[0] = app_data->clear_color.r;
    app_data->rt_data.scene_data.sky_color[1] = app_data->clear_color.g;
    app_data->rt_data.scene_data.sky_color[2] = app_data->clear_color.b;
    app_data->rt_data.scene_data.sky_color[3] = app_data->clear_color.a;

    dm_render_command_update_storage_buffer(&app_data->lights, sizeof(app_data->lights), app_data->light_buffer, context);

    // render updates
    if(!raster_pipeline_update(context)) return false;
    if(!rt_pipeline_update(app_data->sponza_scene, context))     return false;
    if(!debug_pipeline_update(context))  return false;

    return true;
}

bool dm_application_render(dm_context* context)
{
    application_data* app_data = context->app_data;

    // nuklear buffers
    nuklear_gui_update_buffers(context);

    // raytracing has to happen outside of a render pass
    rt_pipeline_render(context);

    // render after
    dm_render_command_begin_render_pass(app_data->clear_color.r,app_data->clear_color.g,app_data->clear_color.b,app_data->clear_color.a, context);
        quad_texture_render(app_data->rt_data.image, context);
        
        nuklear_gui_render(context);
        debug_pipeline_render(context);
    dm_render_command_end_render_pass(context);

    return true;
}
