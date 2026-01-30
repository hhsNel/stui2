#include "layout/window.h"

int
init_window(struct shm_allocator_pdata *pd, shmptr_of(struct window) win, scrcoord width, scrcoord height)
{
	struct window *pwin;

	pwin = fromshmptr(struct window, *pd, win);
	pwin->ins.root = SHMNULL;
	pwin->ins.x_offset = pwin->ins.y_offset = 0;
	pwin->ins.width = width;
	pwin->ins.height = height;
	pwin->active_element = SHMNULL;

	init_input_ctl(&pwin->input_list);

	return STUI_OK;
}

#include <stdio.h>
void
free_window(struct shm_allocator_pdata *pd, struct window *win)
{
	free_element_tree(pd, &win->ins.root);

	free_input_ctl(pd, toshmptr(*pd, &win->input_list));
}

int
resize_window(struct shm_allocator_pdata *pd, shmptr_of(struct window) win, scrcoord new_width, scrcoord new_height)
{
	struct window *pwin;

	pwin = fromshmptr(struct window, *pd, win);
	pwin->ins.width = new_width;
	pwin->ins.height = new_height;

	return STUI_OK;
}

