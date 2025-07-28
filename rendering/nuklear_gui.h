#ifndef __NUKLEAR_GUI_H__
#define __NUKLEAR_GUI_H__

#include "dm.h"

#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_IO
#include "../lib/Nuklear/nuklear.h"

#define MAX_NUKLEAR_FONTS 5
typedef struct nuklear_context_t
{
    struct nk_context    ctx;
    struct nk_font_atlas atlas;
    struct nk_buffer     cmds;

    struct nk_font* fonts[MAX_NUKLEAR_FONTS];
} nuklear_context;

bool nuklear_gui_init(dm_font_desc* font_descs, uint8_t font_count, dm_context* context);
void nuklear_gui_update_input(dm_context* context);
void nuklear_gui_update_buffers(dm_context* context);
void nuklear_gui_render(dm_context* context);

#endif
