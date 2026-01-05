#ifndef ELEMENTS_H
#define ELEMENTS_H

#include "base/screen.h"
#include "shm/allocator.h"

#define RESIZABLE_COORD_BASE 16
struct coord {
	scrcoord fixed;
	uint8_t resizable;
};

struct rect {
	struct coord x, y;
	struct coord width, height;
};

enum element_type {
	ELEMENT_CUSTOM, /* TODO */
};
enum element_flag {
	ASDF, /* TODO */
};
typedef uint16_t element_z_index;
typedef uint16_t element_flags;
struct element {
	enum element_type type;
	struct rect pos;
	element_flags flags;
	element_z_index z_index;
	void *data;
};

struct element_list_node {
	shmptr_of(struct element_list_node) prev;
	shmptr_of(struct element_list_node) next;
	shmptr_of(struct element) el;
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

int element_list_insert(struct shm_allocator_pdata *pd, shmptr_of(struct element_list) list, shmptr_of(struct element) el);
int element_list_remove(struct shm_allocator_pdata *pd, shmptr_of(struct element_list) list, shmptr_of(struct element) el);
struct element_list *z_index_find_list(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) root, element_z_index index);
int z_index_list_insert(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *root, element_z_index index, shmptr_of(struct element) el);
int z_index_remove(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *root, shmptr_of(struct element) el);

#endif

