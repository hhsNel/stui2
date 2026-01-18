#ifndef LABEL_H
#define LABEL_H

#include <stdarg.h>

#include "layout/element.h"

int  init_element_label(struct shm_allocator_pdata *pd, struct element *el, va_list args);
void free_element_label(struct shm_allocator_pdata *pd, struct element *el);
int  element_draw_label(struct shm_allocator_pdata *pd, struct element *el);

#endif

