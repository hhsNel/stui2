#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <stdint.h>

#include "base/screen.h"
#include "shm/allocator.h"
#include "elements/dispatch.h"

struct scr_rect {
	scrcoord x, y;
	scrcoord width, height;
};

enum element_flag {
	ASDF, /* TODO */
};
typedef uint16_t element_flags;

struct window;
struct element {
	struct rect pos;
	struct scr_rect scr_pos;
	element_z_index z_index;
	shmptr_of(struct screen) render_output;
	element_flags flags;
	shmptr_of(struct stui2_insertable) parent_insertable;

	struct element_data data;
};

struct element_list_node {
	shmptr_of(struct element_list_node) prev;
	shmptr_of(struct element_list_node) next;
	struct element el;
};
struct element_list {
	shmptr_of(struct element_list_node) head;
	shmptr_of(struct element_list_node) tail;
};
struct z_index_node {
	element_z_index index;

	shmptr_of(struct z_index_node) left;
	shmptr_of(struct z_index_node) right;
	uint8_t height;

	struct element_list list;
};

struct stui2_insertable {
	shmptr_of(struct z_index_node) root;
	shmptr_of(struct screen) target_scr;
	scrcoord x_offset, y_offset;
	scrcoord width, height;
};

void element_resize(struct shm_allocator_pdata pd, struct element *el);

void init_element_list(struct element_list *list);
void free_element_list(struct shm_allocator_pdata *pd, struct element_list list);
shmptr_of(struct element) element_list_insert(struct shm_allocator_pdata *pd, shmptr_of(struct element_list) list, struct element el);
int  element_list_remove(struct shm_allocator_pdata *pd, shmptr_of(struct element_list) list, shmptr_of(struct element) el);

void free_element_tree(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *root);
struct element_list *z_index_find_list(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) root, element_z_index index);
shmptr_of(struct element) z_index_list_insert(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *root, element_z_index index, struct element el);
int  z_index_remove(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *root, shmptr_of(struct element) el);

#endif

