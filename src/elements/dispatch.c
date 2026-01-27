#include "dispatch.h"

#include <stdio.h>
#include <stdlib.h>

#include "label.h"

#define _CONCAT2(A,B) A##B
#define CONCAT2(A,B) _CONCAT2(A,B)

#define   DISP_RET(ENUMVAL,SUFFIX,PREFIX,...) \
	case ENUMVAL: return CONCAT2(PREFIX,SUFFIX)(__VA_ARGS__);
#define DISP_NORET(ENUMVAL,SUFFIX,PREFIX,...) \
	case ENUMVAL:        CONCAT2(PREFIX,SUFFIX)(__VA_ARGS__); break;

#define   DISPATCH(TYPE,PREFIX,...) \
	switch(TYPE) { \
		TABLE(DISP_RET,  PREFIX,__VA_ARGS__) \
		default: printf("well, we might be fucked\nTODO\n");exit(1); /* TODO */ \
	}
#define DISPATCH_NORET(TYPE,PREFIX,...) \
	switch(TYPE) { \
		TABLE(DISP_NORET,PREFIX,__VA_ARGS__) \
		default: printf("well, we might be fucked\nTODO\n");exit(1); /* TODO */ \
	}

/* main dispatch table that controls everything */
#define TABLE(MACRO,...) \
	MACRO(ELEMENT_LABEL,  label,  __VA_ARGS__) \
	//MACRO(ELEMENT_CUSTOM, custom, __VA_ARGS__)

int
dispatch_init_element(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, enum stui2_element_type type, va_list args)
{
	struct element *pel;

	pel = fromshmptr(struct element, *pd, el);
	pel->data.type = type;
	DISPATCH(type, init_element_, pd, el, args);
}

void
dispatch_free_element(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	struct element *pel;

	pel = fromshmptr(struct element, *pd, el);
	DISPATCH_NORET(pel->data.type, free_element_, pd, el);
}

int
dispatch_element_draw(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	struct element *pel;

	pel = fromshmptr(struct element, *pd, el);
	DISPATCH(pel->data.type, element_draw_, pd, el);
}

