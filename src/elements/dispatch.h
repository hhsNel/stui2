#ifndef DISPATCH_H
#define DISPATCH_H

#include <stdarg.h>

#include "shm/allocator.h"
#include "base/screen.h"

struct element_data {
	enum stui2_element_type type;
	shmptr_of(void) type_data;
};

struct element;

int  dispatch_init_element(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, enum stui2_element_type type, va_list args);
void dispatch_free_element(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);
int  dispatch_element_draw(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);
int  dispatch_element_resize(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);

#endif

