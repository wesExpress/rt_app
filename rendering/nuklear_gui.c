#define NK_IMPLEMENTATION 
#include "nuklear_gui.h"

#include "../app.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_INDEX_BUFFER  128 * 1024

#define MAX_NUKLEAR_FONTS    5
#define MAX_NUKLEAR_VERTICES 1000
typedef struct nuklear_vertex_t
{
    dm_vec2 position;
    dm_vec2 tex_coords;
    dm_vec4 color;
} nuklear_vertex;

typedef struct nuklear_resource_indices_t
{
    uint32_t camera_data_index;
    uint32_t font_texture_index;
    uint32_t sampler_index;
} nuklear_resource_indices;

typedef struct nuklear_internal_data_t
{
    dm_resource_handle vb, ib, pipeline;
    dm_resource_handle cb;
    dm_resource_handle sampler, font_texture;

    struct nk_font* nk_fonts[MAX_NUKLEAR_FONTS];
    uint8_t font_count;

    nuklear_resource_indices resource_indices;

    nuklear_vertex vertices[MAX_VERTEX_BUFFER];
    uint16_t indices[MAX_INDEX_BUFFER];

    struct nk_draw_null_texture null_texture;

    uint32_t screen_width, screen_height;
    dm_mat4 ortho;
} nuklear_gui_data;

void nuklear_clipboard_copy(nk_handle user, const char* text, int len)
{
    NK_UNUSED(user);
    NK_UNUSED(text);
    NK_UNUSED(len);
}

void nuklear_clipboad_paste(nk_handle user, struct nk_text_edit* edit)
{
    NK_UNUSED(user);
    NK_UNUSED(edit);
}

bool nuklear_gui_init(dm_font_desc* font_descs, uint8_t font_count, dm_context* context)
{
    application_data* app_data = context->app_data;
    app_data->nuklear_data = dm_alloc(sizeof(nuklear_gui_data));

    nuklear_gui_data* gui_data = app_data->nuklear_data;

    // === sampler ===
    {
        dm_sampler_desc sampler_desc = {
            .address_w=DM_SAMPLER_ADDRESS_MODE_BORDER,
            .address_v=DM_SAMPLER_ADDRESS_MODE_BORDER,
            .address_u=DM_SAMPLER_ADDRESS_MODE_BORDER
        };

        if(!dm_renderer_create_sampler(sampler_desc, &gui_data->sampler, context)) return false;
    }

    // === constant buffer ===
    {
        dm_mat_ortho(0,(float)context->renderer.width, (float)context->renderer.height,0, -1,1, gui_data->ortho);

        dm_constant_buffer_desc cb_desc = {
            .size=sizeof(dm_mat4),
            .data=&gui_data->ortho
        };

        if(!dm_renderer_create_constant_buffer(cb_desc, &gui_data->cb, context)) return false;

        gui_data->screen_width  = context->renderer.width;
        gui_data->screen_height = context->renderer.height;
    }

    // === vertex and index buffers ===
    {
        dm_vertex_buffer_desc vb_desc = { 
            .stride=sizeof(nuklear_vertex),
            .size=sizeof(gui_data->vertices)
        };

        if(!dm_renderer_create_vertex_buffer(vb_desc, &gui_data->vb, context)) return false;

        dm_index_buffer_desc ib_desc = {
            .size=sizeof(gui_data->indices),
            .index_type=DM_INDEX_BUFFER_INDEX_TYPE_UINT16,
        };

        if(!dm_renderer_create_index_buffer(ib_desc, &gui_data->ib, context)) return false;
    }

    // === pipeline ===
    {
        dm_input_element_desc position_element = {
            .name="POSITION",
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_2,
            .stride=sizeof(nuklear_vertex),
            .offset=offsetof(nuklear_vertex, position),
        };

        dm_input_element_desc tex_coords_element = {
            .name="TEX_COORDS",
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_2,
            .stride=sizeof(nuklear_vertex),
            .offset=offsetof(nuklear_vertex, tex_coords),
        };

        dm_input_element_desc color_element = {
            .name="COLOR",
            .class=DM_INPUT_ELEMENT_CLASS_PER_VERTEX,
            .format=DM_INPUT_ELEMENT_FORMAT_FLOAT_4,
            .stride=sizeof(nuklear_vertex),
            .offset=offsetof(nuklear_vertex, color),
        };
        
        dm_raster_input_assembler_desc input_assembler = {
            .input_elements={ position_element,tex_coords_element,color_element }, .input_element_count=3,
            .topology=DM_INPUT_TOPOLOGY_TRIANGLE_LIST
        };

        dm_shader_desc vertex_shader_desc = {
#ifdef DM_DIRECTX12
            .path="assets/shaders/nuklear_gui_vertex.cso"
#elif defined(DM_VULKAN)
            .path="assets/shaders/nuklear_gui_vertex.spv"
#endif
        };

        dm_shader_desc pixel_shader_desc = {
#ifdef DM_DIRECTX12
            .path="assets/shaders/nuklear_gui_pixel.cso"
#elif defined(DM_VULKAN)
            .path="assets/shaders/nuklear_gui_pixel.spv"
#endif
        };

        dm_rasterizer_desc rasterizer_desc = {
            .cull_mode=DM_RASTERIZER_CULL_MODE_NONE, .front_face=DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE, .polygon_fill=DM_RASTERIZER_POLYGON_FILL_FILL,
            .vertex_shader_desc=vertex_shader_desc, .pixel_shader_desc=pixel_shader_desc
        };

        dm_viewport viewport = {
            .type=DM_VIEWPORT_TYPE_DEFAULT
        };

        dm_scissor scissor = {
            .type=DM_SCISSOR_TYPE_DEFAULT
        };

        dm_depth_stencil_desc depth_stencil = {
            .depth=false,
            .stencil=false
        };

        dm_raster_pipeline_desc pipeline_desc = {
            pipeline_desc.input_assembler=input_assembler,.rasterizer=rasterizer_desc,
            .viewport=viewport, .scissor=scissor, .depth_stencil=depth_stencil
        };

        if(!dm_renderer_create_raster_pipeline(pipeline_desc, &gui_data->pipeline, context)) return false;
    }

    // === nuklear initialization ===
    {
        nk_buffer_init_default(&app_data->nk_context.cmds);

        nk_init_default(&app_data->nk_context.ctx, 0);

        app_data->nk_context.ctx.clip.copy     = nuklear_clipboard_copy;
        app_data->nk_context.ctx.clip.paste    = nuklear_clipboad_paste;
        app_data->nk_context.ctx.clip.userdata = nk_handle_ptr(0);

        // style
        struct nk_color table[NK_COLOR_COUNT];
        table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 215);
        table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
        table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
        nk_style_from_table(&app_data->nk_context.ctx, table);
        
        // fonts
        nk_font_atlas_init_default(&app_data->nk_context.atlas);
        nk_font_atlas_begin(&app_data->nk_context.atlas);


        for(uint8_t i=0; i<font_count; i++)
        {
            gui_data->nk_fonts[i] = nk_font_atlas_add_from_file(&app_data->nk_context.atlas, font_descs[0].path, font_descs[0].size, 0);

            int w,h;
            const void* data = nk_font_atlas_bake(&app_data->nk_context.atlas, &w,&h, NK_FONT_ATLAS_RGBA32);

            dm_texture_desc texture_desc = { 
                .width=w, .height=h,
                .format=DM_TEXTURE_FORMAT_BYTE_4_UNORM,.n_channels=4,
                .sampler=gui_data->sampler,
                .data=data
            };

            if(!dm_renderer_create_texture(texture_desc, &gui_data->font_texture, context)) return false;
        }

        nk_font_atlas_end(&app_data->nk_context.atlas, nk_handle_ptr(0), &gui_data->null_texture);

        nk_style_set_font(&app_data->nk_context.ctx, &gui_data->nk_fonts[0]->handle);
    }

    return true;
}

#ifndef DM_DEBUG
DM_INLINE
#endif
enum nk_keys convert_key_type(dm_key_code key)
{
    switch(key)
    {
        case DM_KEY_LSHIFT: return NK_KEY_SHIFT;

        case DM_KEY_ENTER:     return NK_KEY_ENTER;
        case DM_KEY_BACKSPACE: return NK_KEY_BACKSPACE;
        case DM_KEY_DELETE:    return NK_KEY_DEL;
        case DM_KEY_TAB:       return NK_KEY_TAB;

        case DM_KEY_PAGEUP:   return NK_KEY_SCROLL_UP;
        case DM_KEY_PAGEDOWN: return NK_KEY_SCROLL_DOWN;

        case DM_KEY_HOME:     return NK_KEY_TEXT_START;
        case DM_KEY_END:      return NK_KEY_TEXT_END;

        case DM_KEY_UP:    return NK_KEY_UP;
        case DM_KEY_DOWN:  return NK_KEY_DOWN;
        case DM_KEY_LEFT:  return NK_KEY_LEFT;
        case DM_KEY_RIGHT: return NK_KEY_RIGHT;

        default: return NK_KEY_NONE;
    }
}

#ifndef DM_DEBUG
DM_INLINE
#endif
enum nk_buttons convert_button_type(dm_mousebutton_code button)
{
    switch(button)
    {
        case DM_MOUSEBUTTON_L: return NK_BUTTON_LEFT;
        case DM_MOUSEBUTTON_R: return NK_BUTTON_RIGHT;
        case DM_MOUSEBUTTON_M: return NK_BUTTON_MIDDLE;

        case DM_MOUSEBUTTON_DOUBLE: return NK_BUTTON_DOUBLE;

        default: return 0;
    }
}

void nuklear_gui_update_input(dm_context* context)
{
    application_data* app_data = context->app_data;
    struct nk_context* ctx = &app_data->nk_context.ctx;

    nk_input_begin(ctx);
    for(uint32_t i=0; i<context->platform_data.event_list.num; i++)
    {
        dm_event e = context->platform_data.event_list.events[i];

        switch(e.type)
        {
            case DM_EVENT_KEY_DOWN:
            case DM_EVENT_KEY_UP:
            {
                int down = e.type == DM_EVENT_KEY_DOWN ? 1 : 0;

                if(dm_input_is_key_pressed(DM_KEY_LCTRL, context))
                {
                    switch(e.key)
                    {
                        case DM_KEY_U:
                        nk_input_key(ctx, NK_KEY_TEXT_UNDO, down);
                        break;

                        case DM_KEY_R:
                        nk_input_key(ctx, NK_KEY_TEXT_REDO, down);
                        break;

                        case DM_KEY_C:
                        nk_input_key(ctx, NK_KEY_COPY, down);
                        break;

                        case DM_KEY_V:
                        nk_input_key(ctx, NK_KEY_PASTE, down);
                        break;

                        case DM_KEY_LEFT:
                        nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
                        break;

                        case DM_KEY_RIGHT:
                        nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
                        break;

                        default:
                        break;
                    }
                }
                else
                {
                    nk_input_key(ctx, convert_key_type(e.key), down);
                }
            } break;

            case DM_EVENT_MOUSEBUTTON_DOWN:
            case DM_EVENT_MOUSEBUTTON_UP:
            {
                int down = e.type==DM_EVENT_MOUSEBUTTON_DOWN ? 1 : 0;
                uint32_t x,y;
                dm_input_get_mouse_pos(&x,&y, context);

                nk_input_button(ctx, convert_button_type(e.button), x,y, down);
            } break;

            break;

            case DM_EVENT_MOUSE_MOVE:
            {
                uint32_t x,y;
                dm_input_get_mouse_pos(&x,&y, context);

                nk_input_motion(ctx, x,y);
            } break;

            case DM_EVENT_MOUSE_SCROLL:
            {
                float scroll = dm_input_get_mouse_scroll(context);

                nk_input_scroll(ctx, nk_vec2(0,scroll));
            } break;

            default:
            continue;
        }
    }
    nk_input_end(ctx);
}

void nuklear_gui_update_buffers(dm_context* context)
{
    application_data* app_data = context->app_data;
    nuklear_gui_data* gui_data = app_data->nuklear_data;

    // projection matrix
    dm_mat_ortho(0,(float)context->renderer.width, (float)context->renderer.height,0, -1,1, gui_data->ortho);

    dm_render_command_update_constant_buffer(&gui_data->ortho, sizeof(gui_data->ortho), gui_data->cb, context);

    // vertex and index buffers
    struct nk_convert_config config = { 0 };
    NK_STORAGE const struct nk_draw_vertex_layout_element vertex_layout[] = {
        { NK_VERTEX_POSITION, NK_FORMAT_FLOAT,              NK_OFFSETOF(nuklear_vertex, position) },
        { NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT,              NK_OFFSETOF(nuklear_vertex, tex_coords) },
        { NK_VERTEX_COLOR,    NK_FORMAT_R32G32B32A32_FLOAT, NK_OFFSETOF(nuklear_vertex, color) },
        { NK_VERTEX_LAYOUT_END }
    };

    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof(nuklear_vertex);
    config.vertex_alignment = NK_ALIGNOF(nuklear_vertex);
    config.global_alpha = 1.0f;
    config.shape_AA = NK_ANTI_ALIASING_ON;
    config.line_AA = NK_ANTI_ALIASING_ON;
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.tex_null = gui_data->null_texture;

    struct nk_buffer vbuf, ibuf;
    nk_buffer_init_fixed(&vbuf, gui_data->vertices, (size_t)MAX_VERTEX_BUFFER);
    nk_buffer_init_fixed(&ibuf, gui_data->indices, (size_t)MAX_INDEX_BUFFER);
    nk_convert(&app_data->nk_context.ctx, &app_data->nk_context.cmds, &vbuf, &ibuf, &config);

    dm_render_command_update_vertex_buffer(gui_data->vertices, sizeof(gui_data->vertices), gui_data->vb, context);
    dm_render_command_update_index_buffer(gui_data->indices, sizeof(gui_data->indices), gui_data->ib, context);

}

void nuklear_gui_render(dm_context* context)
{
    application_data* app_data = context->app_data;
    nuklear_gui_data* gui_data = app_data->nuklear_data;

    gui_data->resource_indices.camera_data_index  = gui_data->cb.descriptor_index;
    gui_data->resource_indices.font_texture_index = gui_data->font_texture.descriptor_index+1;
    gui_data->resource_indices.sampler_index      = gui_data->sampler.descriptor_index;

    dm_render_command_bind_raster_pipeline(gui_data->pipeline, context);
    dm_render_command_bind_vertex_buffer(gui_data->vb, 0, context);
    dm_render_command_bind_index_buffer(gui_data->ib, context);
    dm_render_command_set_root_constants(0,3,0, &gui_data->resource_indices, context);

    const struct nk_draw_command* cmd;
    uint32_t offset = 0;
    nk_draw_foreach(cmd, &app_data->nk_context.ctx, &app_data->nk_context.cmds)
    {
        if(!cmd->elem_count) continue;

        dm_render_command_draw_instanced_indexed(1,0,cmd->elem_count,offset,0, context);
        offset += cmd->elem_count;
    }
    
    nk_clear(&app_data->nk_context.ctx);
    nk_buffer_clear(&app_data->nk_context.cmds);
}
