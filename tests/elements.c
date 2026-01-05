#include "layout/element.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
	struct shm_allocator_pdata pd;
	shmptr_of(struct z_index_node) root;
	shmptr_of(struct element) el0;
	shmptr_of(struct element) el1;
	shmptr_of(struct element) el2;

	init_shm_allocator(&pd, NULL, 1, 0);

	root = SHMNULL;

	el0 = shm_alloc(&pd, sizeof(struct element)); fromshmptr(struct element, pd, el0)->z_index = 0;
	el1 = shm_alloc(&pd, sizeof(struct element)); fromshmptr(struct element, pd, el1)->z_index = 1;
	el2 = shm_alloc(&pd, sizeof(struct element)); fromshmptr(struct element, pd, el2)->z_index = 2;

	z_index_list_insert(&pd, &root, 0, el0);
	z_index_list_insert(&pd, &root, 1, el1);
	z_index_list_insert(&pd, &root, 2, el2);

	free_shm_allocator(&pd, 1);
}

