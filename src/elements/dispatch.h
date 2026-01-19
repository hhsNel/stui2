#ifndef DISPATCH_H
#define DISPATCH_H

#include <stdarg.h>

#include "shm/allocator.h"
#include "base/screen.h"

enum element_type {
	ELEMENT_LABEL,
	ELEMENT_CUSTOM, /* TODO */
};

enum element_flag {
	ASDF, /* TODO */
};
typedef uint16_t element_flags;

struct element_data {
	enum element_type type;
	element_flags flags;
	shmptr_of(void) type_data;
};

struct element;

int  dispatch_init_element(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, enum element_type type, va_list args);
void dispatch_free_element(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);
int  dispatch_element_draw(struct shm_allocator_pdata *pd, shmptr_of(struct element) el);

#endif

