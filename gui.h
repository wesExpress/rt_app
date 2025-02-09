#ifndef __GUI_H__
#define __GUI_H__

#include "dm.h"

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

#define MAX_GUI_VERTICES 1000

typedef struct gui_font_t
{
    dm_font          font;
    gui_text_vertex  vertices[MAX_GUI_VERTICES];
    uint32_t         count;
    dm_render_handle vb;
} gui_font;

typedef struct gui_data_s_t
{
    dm_render_handle quad_vb, cb;
    dm_render_handle quad_pipe, text_pipe;
    gui_font         font16, font32;

    gui_quad_vertex quad_vertices[MAX_GUI_VERTICES];

    uint32_t   quad_count;
} gui_data_s;

typedef struct gui_style_t
{
    uint8_t text_padding_l, text_padding_r;
    uint8_t text_padding_t, text_padding_b;
    uint8_t window_border_w, window_border_h;

    float window_border_color[4];
    float window_color[4];
} gui_style;

bool gui_init(gui_style style, void** gui_ctxt, dm_context* context);

bool gui_load_font(const char* path, uint8_t font_size, uint8_t* font_index, void* gui_ctxt, dm_context* context);

void gui_draw_quad(float x, float y, float w, float h, float r, float g, float b, float a, void* context);
void gui_draw_text(float x, float y, const char* input_text, float r, float g, float b, float a, uint8_t font_index, void* context);

void gui_render(void* gui_ctxt, dm_context* context);

#endif
