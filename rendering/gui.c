#include "gui.h"

typedef struct gui_resources_t
{
    uint32_t scene_data;
    uint32_t texture;
} gui_resources;

typedef struct gui_quad_vertex_t
{
    float pos[2];
    float color[4];
} gui_quad_vertex;

typedef struct gui_text_vertex_t
{
    float pos[2];
    float uv[2];
    float color[4];
} gui_text_vertex;

#define MAX_FONT_COUNT 5
#define MAX_GUI_VERTICES 1000

typedef struct gui_context_t
{
    dm_resource_handle quad_vb, quad_ib;
    gui_quad_vertex  quad_vertices[MAX_GUI_VERTICES];
    uint32_t         quad_vertex_count;

    dm_resource_handle font_vb[MAX_FONT_COUNT];
    dm_font          fonts[MAX_FONT_COUNT];
    gui_text_vertex  text_vertices[MAX_FONT_COUNT][MAX_GUI_VERTICES];
    uint32_t         text_vertex_count[MAX_FONT_COUNT];
    uint8_t          font_count;

    gui_style style;

    dm_resource_handle quad_pipe, text_pipe, cb;

    gui_resources quad_resources;
    gui_resources text_resources[MAX_FONT_COUNT];

    gui_proj cb_data;
} gui_context;

bool gui_init(gui_style style, uint8_t font_count, void** gui_ctxt, dm_context* context)
{
    *gui_ctxt = dm_alloc(sizeof(gui_context));
    gui_context* c = *gui_ctxt;
    c->style = style;

    DM_LOG_INFO("Initializing gui renderer...");

    if(font_count >= MAX_FONT_COUNT)
    {
        DM_LOG_ERROR("Too many fonts requested. Clamping font count");
        font_count = MAX_FONT_COUNT-1;
    }

    // === vertex buffers ===
    {
        dm_vertex_buffer_desc vb_desc = { 0 };
        vb_desc.stride       = sizeof(gui_text_vertex);
        vb_desc.element_size = sizeof(float);
        vb_desc.size         = sizeof(gui_text_vertex) * MAX_GUI_VERTICES;
        vb_desc.data         = NULL;
        for(uint8_t i=0; i<font_count; i++)
        {
            if(!dm_renderer_create_vertex_buffer(vb_desc, &c->font_vb[i], context)) return false;
        }

        vb_desc.stride = sizeof(gui_quad_vertex);
        vb_desc.size   = sizeof(gui_quad_vertex) * MAX_GUI_VERTICES;
        if(!dm_renderer_create_vertex_buffer(vb_desc, &c->quad_vb, context)) return false;
    }

    // constant buffer
    {
        dm_mat_ortho(0,(float)context->renderer.width, (float)context->renderer.height,0, -1,1, c->cb_data.ortho_proj);
#ifdef DM_DIRECTX12
        dm_mat4_transpose(c->cb_data.ortho_proj, c->cb_data.ortho_proj);
#endif

        dm_constant_buffer_desc cb_desc = { 0 };
        cb_desc.size = sizeof(gui_proj);
        cb_desc.data = c->cb_data.ortho_proj;

        if(!dm_renderer_create_constant_buffer(cb_desc, &c->cb, context)) return false;
    }

    // quad pipeline
    {
        dm_raster_pipeline_desc desc = { 0 };

        dm_input_element_desc* input = desc.input_assembler.input_elements;
        dm_strcpy(input->name, "POSITION");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_2;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(gui_quad_vertex);
        input->offset = offsetof(gui_quad_vertex, pos);

        input++;

        dm_strcpy(input->name, "COLOR");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_4;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(gui_quad_vertex);
        input->offset = offsetof(gui_quad_vertex, color);

        desc.input_assembler.topology = DM_INPUT_TOPOLOGY_TRIANGLE_LIST;

        desc.input_assembler.input_element_count = 2;

        // rasterizer
        desc.rasterizer.cull_mode    = DM_RASTERIZER_CULL_MODE_BACK;
        desc.rasterizer.polygon_fill = DM_RASTERIZER_POLYGON_FILL_FILL;
        desc.rasterizer.front_face   = DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE;
#ifdef DM_DIRECTX12
        dm_strcpy(desc.rasterizer.vertex_shader_desc.path, "assets/quad_vertex.cso");
        dm_strcpy(desc.rasterizer.pixel_shader_desc.path,  "assets/quad_pixel.cso");
#else
        dm_strcpy(desc.rasterizer.vertex_shader_desc.path, "assets/quad_vertex.spv");
        dm_strcpy(desc.rasterizer.pixel_shader_desc.path,  "assets/quad_pixel.spv");
#endif

        desc.viewport.type = DM_VIEWPORT_TYPE_DEFAULT;
        desc.scissor.type  = DM_SCISSOR_TYPE_DEFAULT;

        if(!dm_renderer_create_raster_pipeline(desc, &c->quad_pipe, context)) return false;
    }

    // text pipeline
    {
        dm_raster_pipeline_desc desc = { 0 };

        dm_input_element_desc* input = desc.input_assembler.input_elements;
        dm_strcpy(input->name, "POSITION");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_2;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(gui_text_vertex);
        input->offset = offsetof(gui_text_vertex, pos);

        input++;

        dm_strcpy(input->name, "TEX_COORDS");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_2;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(gui_text_vertex);
        input->offset = offsetof(gui_text_vertex, uv);

        input++;

        dm_strcpy(input->name, "COLOR");
        input->format = DM_INPUT_ELEMENT_FORMAT_FLOAT_4;
        input->class  = DM_INPUT_ELEMENT_CLASS_PER_VERTEX;
        input->stride = sizeof(gui_text_vertex);
        input->offset = offsetof(gui_text_vertex, color);

        desc.input_assembler.topology = DM_INPUT_TOPOLOGY_TRIANGLE_LIST;

        desc.input_assembler.input_element_count = 3;

        // rasterizer
        desc.rasterizer.cull_mode    = DM_RASTERIZER_CULL_MODE_BACK;
        desc.rasterizer.polygon_fill = DM_RASTERIZER_POLYGON_FILL_FILL;
        desc.rasterizer.front_face   = DM_RASTERIZER_FRONT_FACE_COUNTER_CLOCKWISE;
#ifdef DM_DIRECTX12
        dm_strcpy(desc.rasterizer.vertex_shader_desc.path, "assets/gui_vertex.cso");
        dm_strcpy(desc.rasterizer.pixel_shader_desc.path,  "assets/gui_pixel.cso");
#else
        dm_strcpy(desc.rasterizer.vertex_shader_desc.path, "assets/gui_vertex.spv");
        dm_strcpy(desc.rasterizer.pixel_shader_desc.path,  "assets/gui_pixel.spv");
#endif

        desc.viewport.type = DM_VIEWPORT_TYPE_DEFAULT;
        desc.scissor.type  = DM_SCISSOR_TYPE_DEFAULT;

        desc.sampler = true;

        // depth stencil
        desc.depth_stencil.depth = false;

        if(!dm_renderer_create_raster_pipeline(desc, &c->text_pipe, context)) return false;
    }

    return true;
}

bool gui_load_font(const char* path, uint8_t font_size, uint8_t* font_index, void* gui_ctxt, dm_context* context)
{
    gui_context* c = gui_ctxt;

    if(!dm_renderer_load_font(path, font_size, &c->fonts[c->font_count], context)) return false;

    *font_index = c->font_count++; 

    return true;
}

void gui_draw_quad_border(float x, float y, float w, float h, const float* color, float* border_color, void* context)
{
    gui_context* c = context;

    gui_style style = c->style;

    gui_quad_vertex b0 = {
        { x - style.window_border_w,y - style.window_border_h }, 
        { border_color[0],border_color[1],border_color[2],border_color[3] }
    };
    gui_quad_vertex b1 = {
        { x + w + style.window_border_w,y - style.window_border_h }, 
        { border_color[0],border_color[1],border_color[2],border_color[3] }
    };
    gui_quad_vertex b2 = {
        { x + w + style.window_border_w,y + h + style.window_border_h }, 
        { border_color[0],border_color[1],border_color[2],border_color[3] }
    };
    gui_quad_vertex b3 = {
        { x - style.window_border_w,y +h + style.window_border_h }, 
        { border_color[0],border_color[1],border_color[2],border_color[3] }
    };

    gui_quad_vertex v0 = {
        { x, y },
        { color[0],color[1],color[2],color[3] }
    };
    gui_quad_vertex v1 = {
        { x + w, y }, 
        { color[0],color[1],color[2],color[3] }
    };
    gui_quad_vertex v2 = { 
        { x + w, y + h}, 
        { color[0],color[1],color[2],color[3] }
    };
    gui_quad_vertex v3 = { 
        { x, y + h }, 
        { color[0],color[1],color[2],color[3] }
    };

    c->quad_vertices[c->quad_vertex_count++] = b0;
    c->quad_vertices[c->quad_vertex_count++] = b2;
    c->quad_vertices[c->quad_vertex_count++] = b1;

    c->quad_vertices[c->quad_vertex_count++] = b0;
    c->quad_vertices[c->quad_vertex_count++] = b3;
    c->quad_vertices[c->quad_vertex_count++] = b2;

    c->quad_vertices[c->quad_vertex_count++] = v0;
    c->quad_vertices[c->quad_vertex_count++] = v2;
    c->quad_vertices[c->quad_vertex_count++] = v1;

    c->quad_vertices[c->quad_vertex_count++] = v0;
    c->quad_vertices[c->quad_vertex_count++] = v3;
    c->quad_vertices[c->quad_vertex_count++] = v2;
}

void gui_draw_quad(float x, float y, float w, float h, const float* color, void* context)
{
    gui_context* c = context;

    gui_style style = c->style;

    gui_quad_vertex v0 = {
        { x, y },
        { color[0],color[1],color[2],color[3] }
    };
    gui_quad_vertex v1 = {
        { x + w, y }, 
        { color[0],color[1],color[2],color[3] }
    };
    gui_quad_vertex v2 = { 
        { x + w, y + h}, 
        { color[0],color[1],color[2],color[3] }
    };
    gui_quad_vertex v3 = { 
        { x, y + h }, 
        { color[0],color[1],color[2],color[3] }
    };

    c->quad_vertices[c->quad_vertex_count++] = v0;
    c->quad_vertices[c->quad_vertex_count++] = v2;
    c->quad_vertices[c->quad_vertex_count++] = v1;

    c->quad_vertices[c->quad_vertex_count++] = v0;
    c->quad_vertices[c->quad_vertex_count++] = v3;
    c->quad_vertices[c->quad_vertex_count++] = v2;
}

void gui_draw_text(float x, float y, const char* input_text, const float* color, uint8_t font_index, void* context)
{
    gui_context* c = context;

    const char* text   = input_text;
    const char* runner = input_text;
    float text_len = 0.f;
    float max_h    = 0.f;

    uint32_t num_glyphs = 0;
    float xf = x;
    float yf = y;
    while(*runner)
    {
        if(*runner >= 32 && *runner <= 127)
        {
            dm_font_aligned_quad quad = dm_font_get_aligned_quad(c->fonts[font_index], *runner, &xf,&yf);

            text_len += quad.x1 - quad.x0;
            max_h = DM_MAX(max_h, (quad.y1 - quad.y0));
            num_glyphs++;
        }
        runner++;
    }
    num_glyphs *= 6;

    xf = x;
    yf = y;

    while(*text)
    {
        if(*text >= 32 && *text <= 127)
        {
            dm_font_aligned_quad quad = dm_font_get_aligned_quad(c->fonts[font_index], *text, &xf,&yf);

            gui_text_vertex v0 = {
                { quad.x0,quad.y0+max_h }, 
                { quad.s0,quad.t0 }, 
                { color[0],color[1],color[2],color[3] }
            };
            gui_text_vertex v1 = {
                { quad.x1,quad.y0+max_h }, 
                { quad.s1,quad.t0 }, 
                { color[0],color[1],color[2],color[3] }
            };
            gui_text_vertex v2 = {
                { quad.x1,quad.y1+max_h }, 
                { quad.s1,quad.t1 }, 
                { color[0],color[1],color[2],color[3] }
            };
            gui_text_vertex v3 = {
                { quad.x0,quad.y1+max_h }, 
                { quad.s0,quad.t1 }, 
                { color[0],color[1],color[2],color[3] }
            };

            c->text_vertices[font_index][c->text_vertex_count[font_index]++] = v0;
            c->text_vertices[font_index][c->text_vertex_count[font_index]++] = v2;
            c->text_vertices[font_index][c->text_vertex_count[font_index]++] = v1;

            c->text_vertices[font_index][c->text_vertex_count[font_index]++] = v0;
            c->text_vertices[font_index][c->text_vertex_count[font_index]++] = v3;
            c->text_vertices[font_index][c->text_vertex_count[font_index]++] = v2;
        }

        text++;
    }
}

void gui_update_buffers(void* gui_ctxt, dm_context* context)
{
    gui_context* c = gui_ctxt;

    dm_render_command_update_vertex_buffer(c->quad_vertices, sizeof(c->quad_vertices), c->quad_vb, context);

    for(uint32_t i=0; i<c->font_count; i++)
    {
        if(c->text_vertex_count[i]==0) continue;

        dm_render_command_update_vertex_buffer(c->text_vertices[i], sizeof(c->text_vertices[i]), c->font_vb[i], context);
    }
}

void gui_render(void* gui_ctxt, dm_context* context)
{
    gui_context* c = gui_ctxt;

    c->quad_resources.scene_data = c->cb.descriptor_index;

    // quads
    if(c->quad_vertex_count)
    {
        dm_render_command_bind_raster_pipeline(c->quad_pipe, context);
        dm_render_command_set_root_constants(0,2,0, &c->quad_resources, context);
        dm_render_command_bind_vertex_buffer(c->quad_vb, 0, context);
        dm_render_command_draw_instanced(1,0, c->quad_vertex_count,0, context);

        c->quad_vertex_count = 0;
    }

    // text rendering
    dm_render_command_bind_raster_pipeline(c->text_pipe, context);
    
    // fonts
    for(uint8_t i=0; i<c->font_count; i++)
    {
        if(c->text_vertex_count[i]==0) continue;

        c->text_resources[i].scene_data = c->cb.descriptor_index;
        c->text_resources[i].texture    = c->fonts[i].texture_handle.descriptor_index;

        dm_render_command_set_root_constants(0,2,0, &c->text_resources[i], context);
        dm_render_command_bind_vertex_buffer(c->font_vb[i], 0, context);
        dm_render_command_draw_instanced(1,0, c->text_vertex_count[i],0, context);

        c->text_vertex_count[i] = 0;
    }
}

