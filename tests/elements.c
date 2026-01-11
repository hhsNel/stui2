#include "layout/element.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void print_tree(struct shm_allocator_pdata pd, struct z_index_node *root);

int main(int argc, char **argv) {
	struct shm_allocator_pdata pd;
	shmptr_of(struct z_index_node) root;
	shmptr_of(struct element) el0;
	shmptr_of(struct element) el1;
	shmptr_of(struct element) el2;
	shmptr_of(struct element) el3;
	shmptr_of(struct element) el4;
	shmptr_of(struct element) el5;
	shmptr_of(struct element) el6;
	struct z_index_node *proot;

	init_shm_allocator(&pd, NULL, 1, 0);

	root = SHMNULL;

	el0 = shm_alloc(&pd, sizeof(struct element)); fromshmptr(struct element, pd, el0)->z_index = 0;
	el1 = shm_alloc(&pd, sizeof(struct element)); fromshmptr(struct element, pd, el1)->z_index = 1;
	el2 = shm_alloc(&pd, sizeof(struct element)); fromshmptr(struct element, pd, el2)->z_index = 2;

	z_index_list_insert(&pd, &root, 0, el0);
	z_index_list_insert(&pd, &root, 1, el1);
	z_index_list_insert(&pd, &root, 2, el2);

	proot = fromshmptr(struct z_index_node, pd, root);
	print_tree(pd, proot);
	printf("---\n");

	el3 = shm_alloc(&pd, sizeof(struct element)); fromshmptr(struct element, pd, el3)->z_index = 3;
	el4 = shm_alloc(&pd, sizeof(struct element)); fromshmptr(struct element, pd, el4)->z_index = 4;
	el5 = shm_alloc(&pd, sizeof(struct element)); fromshmptr(struct element, pd, el5)->z_index = 5;
	el6 = shm_alloc(&pd, sizeof(struct element)); fromshmptr(struct element, pd, el6)->z_index = 6;

	z_index_list_insert(&pd, &root, 3, el3);
	z_index_list_insert(&pd, &root, 4, el4);
	z_index_list_insert(&pd, &root, 5, el5);
	z_index_list_insert(&pd, &root, 6, el6);

	proot = fromshmptr(struct z_index_node, pd, root);
	print_tree(pd, proot);
	printf("---\n");

	z_index_remove(&pd, &root, el0);
	z_index_remove(&pd, &root, el1);
	z_index_remove(&pd, &root, el2);
	z_index_remove(&pd, &root, el3);

	proot = fromshmptr(struct z_index_node, pd, root);
	print_tree(pd, proot);
	printf("---\n");

	fromshmptr(struct element, pd, el0)->z_index = 3;
	fromshmptr(struct element, pd, el1)->z_index = 4;
	fromshmptr(struct element, pd, el2)->z_index = 5;
	fromshmptr(struct element, pd, el3)->z_index = 6;

	z_index_list_insert(&pd, &root, 3, el0);
	z_index_list_insert(&pd, &root, 4, el1);
	z_index_list_insert(&pd, &root, 5, el2);
	z_index_list_insert(&pd, &root, 6, el3);

	proot = fromshmptr(struct z_index_node, pd, root);
	print_tree(pd, proot);

	free_shm_allocator(pd, 1);
}

void
print_tree(struct shm_allocator_pdata pd, struct z_index_node *root)
{
	shmptr_of(struct z_index_node) left, right;
	if(root) {
		left = root->left;
		right = root->right;

		print_tree(pd, fromshmptr(struct z_index_node, pd, left));
		printf("height: %u, index: %u, head: %u, tail: %u\n", (unsigned int)root->height, (unsigned int)root->index, (unsigned int)root->list.head, (unsigned int)root->list.tail);
		print_tree(pd, fromshmptr(struct z_index_node, pd, right));
	}
}

