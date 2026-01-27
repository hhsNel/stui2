#include "stui2.h"

#include "shm/shm.h"
#include "shm/allocator.h"
#include "layout/window.h"
#include "base/dblbuf.h"

#include <stdlib.h>

struct stui2 {
	struct shm_allocator_pdata pd;
	int is_parent;
	shmptr_of(struct window) main_win;
	union {
		struct {
			shmptr_of(struct dblbuf) output_db;
		} parent_data;
		struct {
		} child_data;
	};
};

struct common_data {
};

static int r_render_z_index(struct shm_allocator_pdata *pd, shmptr_of(struct screen) scr, shmptr_of(struct z_index_node) node);

struct stui2 *
init_stui2()
{
	struct stui2 *stui2;
	struct window *pwin;
	struct dblbuf *pdb;

	stui2 = malloc(sizeof(struct stui2));
	if(! stui2) {
		return NULL;
	}
	stui2->is_parent = shm_is_parent();
	if(init_shm_allocator(&stui2->pd, NULL, stui2->is_parent, sizeof(struct common_data)) != STUI_OK) {
		return NULL;
	}

	if(stui2->is_parent) {
		stui2->main_win = shm_alloc(&stui2->pd, sizeof(struct window));
		/*                                         TODO TODO */
		if(init_window(&stui2->pd, stui2->main_win, 32, 32) != STUI_OK) {
			return NULL;
		}
		stui2->parent_data.output_db = shm_alloc(&stui2->pd, sizeof(struct dblbuf));
		/*                                               TODO TODO */
		if(init_dblbuf(&stui2->pd, stui2->parent_data.output_db, 32, 32) != STUI_OK) {
			return NULL;
		}
		pwin = fromshmptr(struct window, stui2->pd, stui2->main_win);
		pdb = fromshmptr(struct dblbuf, stui2->pd, stui2->parent_data.output_db);
		pwin->ins.target_scr = toshmptr(stui2->pd, &pdb->cur_scr);
	} else {
		/* TODO */
		exit(1);
	}

	return stui2;
}

int
exit_stui2(struct stui2 *stui2)
{
	struct window *pwin;

	pwin = fromshmptr(struct window, stui2->pd, stui2->main_win);

	free_window(&stui2->pd, pwin);
	if(free_shm_allocator(stui2->pd, stui2->is_parent) != STUI_OK) {
		return STUI_ERR;
	}

	free(stui2);

	return STUI_OK;
}

stui2_window
stui2_main_window(struct stui2 *stui2)
{
	return stui2->main_win;
}

stui2_insertable
stui2_win_get_insertable(struct stui2 *stui2, stui2_window win)
{
	struct window *pwin;

	pwin = fromshmptr(struct window, stui2->pd, win);

	return toshmptr(stui2->pd, &pwin->ins);
}

stui2_element
stui2_create_element(struct stui2 *stui2, stui2_insertable ins, element_z_index z_index, enum stui2_element_type type, ...)
{
	va_list args;
	struct element el;
	shmptr_of(struct element) shmel;
	struct stui2_insertable *pins;

	shm_access(&stui2->pd);
	pins = fromshmptr(struct stui2_insertable, stui2->pd, ins);

	el.z_index = z_index;
	el.scr_pos = (struct scr_rect){0};
	el.pos = (struct rect){0};
	el.render_output = pins->target_scr;
	el.flags = 0;
	el.parent_window = SHMNULL /* TODO */;

	shmel = z_index_list_insert(&stui2->pd, &pins->root, z_index, el);
	if(shmel == SHMNULL) {
		shm_leave(&stui2->pd);
		return STUI_INVH;
	}
	pins = fromshmptr(struct stui2_insertable, stui2->pd, ins);

	va_start(args, type);
	dispatch_init_element(&stui2->pd, shmel, type, args);
	va_end(args);

	shm_leave(&stui2->pd);

	return shmel;
}

int
stui2_free_element(struct stui2 *stui2, stui2_element el)
{
	struct element *pel;
	struct window *pwin;
	shmptr_of(struct z_index_node) node;

	shm_access(&stui2->pd);
	
	dispatch_free_element(&stui2->pd, el);

	pel = fromshmptr(struct element, stui2->pd, el);
	pwin = fromshmptr(struct window, stui2->pd, pel->parent_window);
	node = pwin->ins.root;
	if(z_index_remove(&stui2->pd, &node, el) != STUI_OK) {
		shm_leave(&stui2->pd);
		return STUI_ERR;
	}
	pel = fromshmptr(struct element, stui2->pd, el);
	pwin = fromshmptr(struct window, stui2->pd, pel->parent_window);
	pwin->ins.root = node;

	return STUI_OK;
}

void
stui2_set_position(struct stui2 *stui2, stui2_element el, struct rect rect)
{
	struct element *pel;

	pel = fromshmptr(struct element, stui2->pd, el);
	pel->pos = rect;
}

int
stui2_render(struct stui2 *stui2, stui2_window win)
{
	struct window *pwin;

	pwin = fromshmptr(struct window, stui2->pd, win);

	return r_render_z_index(&stui2->pd, pwin->ins.target_scr, pwin->ins.root);
}

int
stui2_flush(struct stui2 *stui2)
{
	if(stui2->is_parent) {
		return dump_dblbuf(&stui2->pd, stui2->parent_data.output_db, STDOUT_FILENO);
	}
	return STUI_OK;
}

static int
r_render_z_index(struct shm_allocator_pdata *pd, shmptr_of(struct screen) scr, shmptr_of(struct z_index_node) node)
{
	struct z_index_node *pnode;
	struct element_list_node *plist;
	struct element *pelement;

	if(node == SHMNULL) {
		return STUI_OK;
	}

	shm_access(pd);

	pnode = fromshmptr(struct z_index_node, *pd, node);
	if(r_render_z_index(pd, scr, pnode->left) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}

	plist = fromshmptr(struct element_list_node, *pd, pnode->list.head);
	while(plist != SHMNULL) {
		pelement = &plist->el;
		if(dispatch_element_draw(pd, toshmptr(*pd, pelement)) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}

		plist = fromshmptr(struct element_list_node, *pd, plist->next);
	}

	if(r_render_z_index(pd, scr, pnode->right) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}

	shm_leave(pd);
	return STUI_OK;
}

