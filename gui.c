#include "gui.h"

#define MAX_FONT_COUNT 5

typedef struct gui_context_t
{
    dm_render_handle quad_vb, quad_ib;
    gui_quad_vertex  quad_vertices[MAX_GUI_VERTICES];
    uint32_t         quad_vertex_count;

    dm_render_handle font_vb[MAX_FONT_COUNT];
    dm_font          fonts[MAX_FONT_COUNT];
    gui_text_vertex  text_vertices[MAX_FONT_COUNT][MAX_GUI_VERTICES];
    uint32_t         text_vertex_count[MAX_FONT_COUNT];
    uint8_t          font_count;

    gui_style style;

    dm_render_handle quad_pipe, text_pipe, cb;
} gui_context;

bool gui_init(gui_style style, void** gui_ctxt, dm_context* context)
{
    *gui_ctxt = dm_alloc(sizeof(gui_context));
    gui_context* c = *gui_ctxt;
    c->style = style;

    dm_vertex_buffer_desc vb_desc = { 0 };
    vb_desc.stride       = sizeof(gui_text_vertex);
    vb_desc.element_size = sizeof(float);
    vb_desc.size         = sizeof(gui_text_vertex) * MAX_GUI_VERTICES;
    vb_desc.data         = NULL;
    for(uint8_t i=0; i<MAX_FONT_COUNT; i++)
    {
        if(!dm_renderer_create_vertex_buffer(vb_desc, &c->font_vb[i], context)) return false;
    }

    vb_desc.stride = sizeof(gui_quad_vertex);
    vb_desc.size   = sizeof(gui_quad_vertex) * MAX_GUI_VERTICES;
    if(!dm_renderer_create_vertex_buffer(vb_desc, &c->quad_vb, context)) return false;

    dm_mat4 gui_proj = { 0 };
    dm_mat_ortho(0,(float)context->renderer.width, (float)context->renderer.height,0, -1,1, gui_proj);
#ifdef DM_DIRECTX12
    dm_mat4_transpose(gui_proj, gui_proj);
#endif

    dm_constant_buffer_desc cb_desc = { 0 };
    cb_desc.size = sizeof(dm_mat4);
    cb_desc.data = gui_proj;

    if(!dm_renderer_create_constant_buffer(cb_desc, &c->cb, context)) return false;

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
        dm_strcpy(desc.rasterizer.vertex_shader_desc.path, "assets/quad_vertex.cso");
        dm_strcpy(desc.rasterizer.pixel_shader_desc.path,  "assets/quad_pixel.cso");

        desc.viewport.type = DM_VIEWPORT_TYPE_DEFAULT;
        desc.scissor.type  = DM_SCISSOR_TYPE_DEFAULT;

        // descriptors
        desc.descriptor_group[0].ranges[0].type       = DM_DESCRIPTOR_RANGE_TYPE_CONSTANT_BUFFER;
        desc.descriptor_group[0].ranges[0].count      = 1;
        desc.descriptor_group[0].flags                = DM_DESCRIPTOR_GROUP_FLAG_VERTEX_SHADER;

        desc.descriptor_group[0].range_count = 1;

        desc.descriptor_group_count = 1;

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
        input->offset = offsetof(gui_text_vertex, pos);

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

void gui_draw_quad(float x, float y, float w, float h, float r, float g, float b, float a, void* context)
{
    gui_context* c = context;

    gui_quad_vertex v0 = {
        { x, y }, { r,g,b,a }
    };
    gui_quad_vertex v1 = {
        { x + w, y }, { r,g,b,a }
    };
    gui_quad_vertex v2 = { 
        { x + w, y + h}, { r,g,b,a }
    };
    gui_quad_vertex v3 = { 
        { x, y + h }, { r,g,b,a }
    };

    c->quad_vertices[c->quad_vertex_count++] = v0;
    c->quad_vertices[c->quad_vertex_count++] = v2;
    c->quad_vertices[c->quad_vertex_count++] = v1;

    c->quad_vertices[c->quad_vertex_count++] = v0;
    c->quad_vertices[c->quad_vertex_count++] = v3;
    c->quad_vertices[c->quad_vertex_count++] = v2;
}

void gui_draw_text(float x, float y, const char* input_text, float r, float g, float b, float a, uint8_t font_index, void* context)
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
                { quad.x0,quad.y0+max_h }, { quad.s0,quad.t0 }, { r,g,b,a }
            };
            gui_text_vertex v1 = {
                { quad.x1,quad.y0+max_h }, { quad.s1,quad.t0 }, { r,g,b,a }
            };
            gui_text_vertex v2 = {
                { quad.x1,quad.y1+max_h }, { quad.s1,quad.t1 }, { r,g,b,a }
            };
            gui_text_vertex v3 = {
                { quad.x0,quad.y1+max_h }, { quad.s0,quad.t1 }, { r,g,b,a }
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

void gui_render(void* gui_ctxt, dm_context* context)
{
    gui_context* c = gui_ctxt;

    dm_render_command_bind_constant_buffer(c->cb, 0, context);

    // quads
    {
        dm_render_command_update_vertex_buffer(c->quad_vertices, sizeof(c->quad_vertices), c->quad_vb, context);

        dm_render_command_bind_raster_pipeline(c->quad_pipe, context);
        dm_render_command_bind_descriptor_group(0, 1, context);

        dm_render_command_bind_vertex_buffer(c->quad_vb, context);
        dm_render_command_draw_instanced(1,0, c->quad_vertex_count,0, context);

        c->quad_vertex_count = 0;
    }

    // text rendering
    dm_render_command_bind_raster_pipeline(c->text_pipe, context);
    dm_render_command_bind_constant_buffer(c->cb, 0, context);
    dm_render_command_bind_descriptor_group(0, 1, context);
    
    // fonts
    for(uint8_t i=0; i<c->font_count; i++)
    {
        dm_render_command_update_vertex_buffer(c->text_vertices[i], sizeof(c->text_vertices[i]), c->font_vb[i], context);
        dm_render_command_bind_vertex_buffer(c->font_vb[i], context);
        dm_render_command_bind_texture(c->fonts[i].texture_handle, 0, context);
        dm_render_command_bind_descriptor_group(1, 1, context);
        dm_render_command_draw_instanced(1,0, c->text_vertex_count[i],0, context);

        c->text_vertex_count[i] = 0;
    }
}
