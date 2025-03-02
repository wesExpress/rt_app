#ifndef __GUI_H__
#define __GUI_H__

#include "dm.h"

typedef struct gui_style_t
{
    uint8_t text_padding_l, text_padding_r;
    uint8_t text_padding_t, text_padding_b;
    uint8_t window_border_w, window_border_h;

    float window_border_color[4];
    float window_color[4];
} gui_style;

bool gui_init(gui_style style, uint8_t font_count, void** gui_ctxt, dm_context* context);

bool gui_load_font(const char* path, uint8_t font_size, uint8_t* font_index, void* gui_ctxt, dm_context* context);

void gui_draw_quad(float x, float y, float w, float h, const float* color, void* context);
void gui_draw_quad_border(float x, float y, float w, float h, const float* color, float* border_color, void* context);
void gui_draw_text(float x, float y, const char* input_text, const float* color, uint8_t font_index, void* context);

void gui_update_buffers(void* gui_ctxt, dm_context* context);
void gui_render(void* gui_ctxt, dm_context* context);

#endif
