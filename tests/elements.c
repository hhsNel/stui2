#include "layout/element.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void print_tree(struct shm_allocator_pdata pd, struct z_index_node *root);

int main(int argc, char **argv) {
	struct shm_allocator_pdata pd;
	shmptr_of(struct z_index_node) root;
	struct element el0; shmptr_of(struct element) shmel0;
	struct element el1; shmptr_of(struct element) shmel1;
	struct element el2; shmptr_of(struct element) shmel2;
	struct element el3; shmptr_of(struct element) shmel3;
	struct element el4; shmptr_of(struct element) shmel4;
	struct element el5; shmptr_of(struct element) shmel5;
	struct element el6; shmptr_of(struct element) shmel6;
	struct z_index_node *proot;

	init_shm_allocator(&pd, NULL, 1, 0);

	root = SHMNULL;

	el0.z_index = 0;
	el1.z_index = 1;
	el1.z_index = 2;

	shmel0 = z_index_list_insert(&pd, &root, 0, el0);
	shmel1 = z_index_list_insert(&pd, &root, 1, el1);
	shmel2 = z_index_list_insert(&pd, &root, 2, el2);

	proot = fromshmptr(struct z_index_node, pd, root);
	print_tree(pd, proot);
	printf("---\n");

	el3.z_index = 3;
	el4.z_index = 4;
	el5.z_index = 5;
	el6.z_index = 6;

	shmel3 = z_index_list_insert(&pd, &root, 3, el3);
	shmel4 = z_index_list_insert(&pd, &root, 4, el4);
	shmel5 = z_index_list_insert(&pd, &root, 5, el5);
	shmel6 = z_index_list_insert(&pd, &root, 6, el6);

	proot = fromshmptr(struct z_index_node, pd, root);
	print_tree(pd, proot);
	printf("---\n");

	z_index_remove(&pd, &root, shmel0);
	z_index_remove(&pd, &root, shmel1);
	z_index_remove(&pd, &root, shmel2);
	z_index_remove(&pd, &root, shmel3);

	proot = fromshmptr(struct z_index_node, pd, root);
	print_tree(pd, proot);
	printf("---\n");

	el0.z_index = 3;
	el1.z_index = 4;
	el2.z_index = 5;
	el3.z_index = 6;

	shmel1 = z_index_list_insert(&pd, &root, 3, el0);
	shmel2 = z_index_list_insert(&pd, &root, 4, el1);
	shmel3 = z_index_list_insert(&pd, &root, 5, el2);
	shmel4 = z_index_list_insert(&pd, &root, 6, el3);

	proot = fromshmptr(struct z_index_node, pd, root);
	print_tree(pd, proot);

	printf(
			"shmel0 = %u\n"
			"shmel1 = %u\n"
			"shmel2 = %u\n"
			"shmel3 = %u\n"
			"shmel4 = %u\n"
			"shmel5 = %u\n"
			"shmel6 = %u\n",
			shmel0, shmel1, shmel2, shmel3, shmel4, shmel5, shmel6);

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

