#ifndef TEXT_H
#define TEXT_H

#include <stdarg.h>

#include "layout/element.h"

int  init_element_text(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, va_list args);
void free_element_text(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);
int  element_draw_text(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);
int  element_resize_text(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);

int text_append(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, struct char_cell cc, char *str);

#endif

