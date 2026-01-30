#ifndef LABEL_H
#define LABEL_H

#include <stdarg.h>

#include "layout/element.h"

int  init_element_label(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, va_list args);
void free_element_label(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);
int  element_draw_label(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);
int  element_resize_label(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);

int  label_set_style(struct shm_allocator_pdata pd, struct element *el, struct char_cell cc);
int  label_set_string(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, char *string);

#endif

