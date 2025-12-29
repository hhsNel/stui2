#include "element.h"

struct element_list_node {
	struct element_list_node *prev, *next;
	struct element el;
};
struct element_list {
	struct element_list_node *head;
	struct element_list_node *tail;
};
struct z_index_node {
	element_z_index index;

	struct z_index_node *parent;
	struct z_index_node *left;
	struct z_index_node *right;
	uint8_t height;

	struct element_list list;
};
struct window {
	struct z_index_node *elements;
};

int element_list_insert(struct element_list *list, struct element el);
int element_list_remove(struct element_list *list, struct element *el);
int z_index_list_insert(struct z_index_node *root, element_z_index index, struct element el);
int z_index_remove(struct z_index_node *root, struct element *el);

