#include "layout/element.h"

typedef uint16_t element_flags;

static uint8_t z_index_height(struct z_index_node *node);
static int8_t z_index_balance_factor(struct shm_allocator_pdata *pd, struct z_index_node *node);
static shmptr_of(struct z_index_node) create_z_index_node(struct shm_allocator_pdata *pd, element_z_index index);
static void z_index_rotate_r (struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *node);
static void z_index_rotate_l (struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *node);

void
init_element_list(struct element_list *list)
{
	list->head = list->tail = SHMNULL;
}

void
free_element_list(struct shm_allocator_pdata *pd, struct element_list list)
{
	shmptr_of(struct element_list_node) node;
	shmptr_of(struct element_list_node) next;
	struct element_list_node *pnode;

	node = list.head;
	while(node != SHMNULL) {
		pnode = fromshmptr(struct element_list_node, *pd, node);
		next = pnode->next;
		shm_free(pd, node);
		node = next;
	}
}

shmptr_of(struct element)
element_list_insert(struct shm_allocator_pdata *pd, shmptr_of(struct element_list) list, struct element el)
{
	shmptr_of(struct element_list_node) node;
	struct element_list_node *pnode;
	struct element_list *plist;

	shm_access(pd);

	node = shm_alloc(pd, sizeof(struct element_list_node));
	if(node == SHMNULL) {
		shm_leave(pd);
		return SHMNULL;
	}
	pnode = fromshmptr(struct element_list_node, *pd, node);
	plist = fromshmptr(struct element_list, *pd, list);
	pnode->prev = SHMNULL;
	pnode->next = plist->head;
	pnode->el = el;
	if(plist->head != SHMNULL) {
		fromshmptr(struct element_list_node, *pd, plist->head)->prev = node;
	}
	plist->head = node;
	if(plist->tail == SHMNULL) {
		plist->tail = node;
	}

	shm_leave(pd);
	return toshmptr(*pd, &pnode->el);
}

int
element_list_remove(struct shm_allocator_pdata *pd, shmptr_of(struct element_list) list, shmptr_of(struct element) el)
{
	shmptr_of(struct element_list_node) node;
	struct element_list_node *pnode;
	struct element_list_node *pnext, *pprev;
	struct element_list *plist;

	shm_access(pd);

	plist = fromshmptr(struct element_list, *pd, list);
	node = shmcontainer_of(el, struct element_list_node, el);
	pnode = fromshmptr(struct element_list_node, *pd, node);
	if(pnode->next != SHMNULL) {
		pnext = fromshmptr(struct element_list_node, *pd, pnode->next);
		pnext->prev = pnode->prev;
	} else {
		plist->tail = pnode->prev;
	}
	if(pnode->prev != SHMNULL) {
		pprev = fromshmptr(struct element_list_node, *pd, pnode->prev);
		pprev->next = pnode->next;
	} else {
		plist->head = pnode->next;
	}

	shm_free(pd, node);

	shm_leave(pd);
	return STUI_OK;
}

void
free_element_tree(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *root)
{
	struct z_index_node *proot;

	if(*root == SHMNULL) {
		return;
	}

	proot = fromshmptr(struct z_index_node, *pd, *root);
	free_element_tree(pd, &proot->left);
	proot = fromshmptr(struct z_index_node, *pd, *root);
	free_element_tree(pd, &proot->right);

	shm_free(pd, *root);
}

struct element_list *
z_index_find_list(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) root, element_z_index index)
{
	struct z_index_node *proot;

	if(root == SHMNULL) {
		return NULL;
	}

	proot = fromshmptr(struct z_index_node, *pd, root);

	if(proot->index > index) {
		return z_index_find_list(pd, proot->left, index);
	}

	if(proot->index < index) {
		return z_index_find_list(pd, proot->right, index);
	}

	return &proot->list;
}

shmptr_of(struct element)
z_index_list_insert(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *root, element_z_index index, struct element el)
{
	struct z_index_node *proot;
	struct z_index_node *pleft, *pright;
	int8_t balance;
	shmptr_of(struct element) element;

	shm_access(pd);

	proot = fromshmptr(struct z_index_node, *pd, *root);
	if(*root == SHMNULL) {
		*root = create_z_index_node(pd, index);
		if(*root == SHMNULL) {
			return SHMNULL;
		}
		proot = fromshmptr(struct z_index_node, *pd, *root);
		element = element_list_insert(pd, toshmptr(*pd, &proot->list), el);
		shm_leave(pd);
		return element;
	} else if(proot->index < index) {
		proot = fromshmptr(struct z_index_node, *pd, *root);
		element = z_index_list_insert(pd, &proot->right, index, el);
		if(element == SHMNULL) {
			shm_leave(pd);
			return SHMNULL;
		}
	} else if(proot->index > index) {
		proot = fromshmptr(struct z_index_node, *pd, *root);
		element = z_index_list_insert(pd, &proot->left, index, el);
		if(element == SHMNULL) {
			shm_leave(pd);
			return SHMNULL;
		}
	} else { /* found node with index */
		proot = fromshmptr(struct z_index_node, *pd, *root);
		element = element_list_insert(pd, toshmptr(*pd, &proot->list), el);
		shm_leave(pd);
		return element;
	}

	proot = fromshmptr(struct z_index_node, *pd, *root);
	pleft = fromshmptr(struct z_index_node, *pd, proot->left);
	pright = fromshmptr(struct z_index_node, *pd, proot->right);
	proot->height = 1 + MAX(z_index_height(pleft), z_index_height(pright));

	balance = z_index_balance_factor(pd, proot);

	if(balance > 1 && index < pleft->index) {
		z_index_rotate_r(pd, root);
	}
	if(balance < -1 && index > pright->index) {
		z_index_rotate_l(pd, root);
	}
	if(balance > 1 && index > pleft->index) {
		z_index_rotate_l(pd, &proot->left);
		z_index_rotate_r(pd, root);
	}
	if(balance < -1 && index < pright->index) {
		z_index_rotate_r(pd, &proot->right);
		z_index_rotate_l(pd, root);
	}
	

	shm_leave(pd);
	return element;
}

int
z_index_remove(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *root, shmptr_of(struct element) el)
{
	struct element_list *plist;
	shmptr_of(struct element_list) list;
	struct element *pel;

	shm_access(pd);

	pel = fromshmptr(struct element, *pd, el);
	plist = z_index_find_list(pd, *root, pel->z_index);
	if(!plist) {
		return STUI_ERR;
	}
	list = toshmptr(*pd, plist);
	if(element_list_remove(pd, list, el) != STUI_OK) return STUI_ERR;

	shm_leave(pd);
	return STUI_OK;
}

static uint8_t
z_index_height(struct z_index_node *node)
{
	if(node == NULL) {
		return 0;
	}

	return node->height;
}

static int8_t
z_index_balance_factor(struct shm_allocator_pdata *pd, struct z_index_node *node)
{
	struct z_index_node *pleft, *pright;

	if(node == NULL) {
		return 0;
	}

	pleft = fromshmptr(struct z_index_node, *pd, node->left);
	pright = fromshmptr(struct z_index_node, *pd, node->right);
	return z_index_height(pleft) - z_index_height(pright);
}

static shmptr_of(struct z_index_node)
create_z_index_node(struct shm_allocator_pdata *pd, element_z_index index)
{
	shmptr_of(struct z_index_node) node;
	struct z_index_node *pnode;

	shm_access(pd);
	
	node = shm_alloc(pd, sizeof(struct z_index_node));
	if(node == SHMNULL) {
		shm_leave(pd);
		return SHMNULL;
	}
	pnode = fromshmptr(struct z_index_node, *pd, node);

	pnode->index = index;
	pnode->left = pnode->right = SHMNULL;
	pnode->height = 1;
	pnode->list.head = pnode->list.tail = SHMNULL;

	shm_leave(pd);
	return node;
}

static void
z_index_rotate_r (struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *node)
{
	struct z_index_node *pnode, *pleft, *pright, *pleftright, *pleftleft;
	shmptr_of(struct z_index_node) left, leftright, leftleft;

	pnode = fromshmptr(struct z_index_node, *pd, *node);
	left = pnode->left;
	pleft = fromshmptr(struct z_index_node, *pd, left);
	pright = fromshmptr(struct z_index_node, *pd, pnode->right);
	leftright = pleft->right;
	leftleft = pleft->left;
	pleftright = fromshmptr(struct z_index_node, *pd, leftright);
	pleftleft = fromshmptr(struct z_index_node, *pd, leftleft);

	pleft->right = *node;
	pnode->left = leftright;

	pnode->height = 1 + MAX(z_index_height(pleftright), z_index_height(pright));
	pleft->height = 1 + MAX(z_index_height(pleftleft), z_index_height(pnode));

	*node = left;
}

static void
z_index_rotate_l (struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) *node)
{
	struct z_index_node *pnode, *pleft, *pright, *prightleft, *prightright;
	shmptr_of(struct z_index_node) right, rightright, rightleft;

	pnode = fromshmptr(struct z_index_node, *pd, *node);
	right = pnode->right;
	pleft = fromshmptr(struct z_index_node, *pd, pnode->left);
	pright = fromshmptr(struct z_index_node, *pd, right);
	rightright = pright->right;
	rightleft = pright->left;
	prightright = fromshmptr(struct z_index_node, *pd, rightright);
	prightleft = fromshmptr(struct z_index_node, *pd, rightleft);

	pright->left = *node;
	pnode->right = rightleft;

	pnode->height = 1 + MAX(z_index_height(pleft), z_index_height(prightleft));
	pright->height = 1 + MAX(z_index_height(pnode), z_index_height(prightright));

	*node = right;
}

