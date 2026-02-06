#ifndef DRAW_H
#define DRAW_H

#include <stdarg.h>

#include "layout/element.h"

int  init_element_draw(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, va_list args);
void free_element_draw(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);
int  element_draw_draw(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);
int  element_resize_draw(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);

void draw_setcc(struct shm_allocator_pdata pd, shmptr_of(struct element) el, scrcoord x, scrcoord y, struct char_cell cc);
void draw_mkline(struct shm_allocator_pdata pd, shmptr_of(struct element) el, scrcoord x0, scrcoord y0, scrcoord x1, scrcoord y1, struct char_cell fill);
void draw_rect(struct shm_allocator_pdata pd, shmptr_of(struct element) el, scrcoord x, scrcoord y, scrcoord width, scrcoord height, struct char_cell fill);

#endif

