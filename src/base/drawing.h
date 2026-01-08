#ifndef DRAWING_H
#define DRAWING_H

#include "base/screen.h"
#include "base/dblbuf.h"

int draw_line_screen(struct shm_allocator_pdata pd, struct screen scr, scrcoord x0, scrcoord y0, scrcoord x1, scrcoord y1, struct char_cell fill);
int draw_line_dblbuf(struct shm_allocator_pdata pd, struct dblbuf *db,                                       scrcoord x0, scrcoord y0, scrcoord x1, scrcoord y1, struct char_cell fill);

int fill_rect_screen(struct shm_allocator_pdata pd, struct screen scr, scrcoord x, scrcoord y, scrcoord w, scrcoord h, struct char_cell fill);
int fill_rect_dblbuf(struct shm_allocator_pdata pd, struct dblbuf *db,                                       scrcoord x, scrcoord y, scrcoord w, scrcoord h, struct char_cell fill);

int write_string_screen(struct shm_allocator_pdata pd, struct screen scr, scrcoord x, scrcoord y, scrcoord w, scrcoord h, char *str, struct char_cell ex);
int write_string_dblbuf(struct shm_allocator_pdata pd, struct dblbuf *db,                                       scrcoord x, scrcoord y, scrcoord w, scrcoord h, char *str, struct char_cell ex);

#endif

