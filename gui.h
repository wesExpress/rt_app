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

void gui_draw_quad(float x, float y, float w, float h, float r, float g, float b, float a, gui_quad_vertex* vertices, uint32_t* vertex_count);
void gui_draw_text(float x, float y, const char* input_text, gui_font* font);

#endif
