#include "gui.h"

void gui_draw_quad(float x, float y, float w, float h, float r, float g, float b, float a, gui_quad_vertex* vertices, uint32_t* vertex_count)
{
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

    uint32_t count = *vertex_count;

    vertices[count++] = v0;
    vertices[count++] = v2;
    vertices[count++] = v1;

    vertices[count++] = v0;
    vertices[count++] = v3;
    vertices[count++] = v2;

    *vertex_count = count;
}

void gui_draw_text(float x, float y, const char* input_text, gui_font* font)
{
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
            dm_font_aligned_quad quad = dm_font_get_aligned_quad(font->font, *runner, &xf,&yf);

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
            dm_font_aligned_quad quad = dm_font_get_aligned_quad(font->font, *text, &xf,&yf);

            gui_text_vertex v0 = {
                { quad.x0,quad.y0+max_h }, { quad.s0,quad.t0 }, { 1.f,1.f,1.f,1.f }
            };
            gui_text_vertex v1 = {
                { quad.x1,quad.y0+max_h }, { quad.s1,quad.t0 }, { 1.f,1.f,1.f,1.f }
            };
            gui_text_vertex v2 = {
                { quad.x1,quad.y1+max_h }, { quad.s1,quad.t1 }, { 1.f,1.f,1.f,1.f }
            };
            gui_text_vertex v3 = {
                { quad.x0,quad.y1+max_h }, { quad.s0,quad.t1 }, { 1.f,1.f,1.f,1.f }
            };

            font->vertices[font->count++] = v0;
            font->vertices[font->count++] = v2;
            font->vertices[font->count++] = v1;

            font->vertices[font->count++] = v0;
            font->vertices[font->count++] = v3;
            font->vertices[font->count++] = v2;
        }

        text++;
    }
}
