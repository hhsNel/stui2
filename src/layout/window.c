#include "layout/window.h"

int
init_window(struct shm_allocator_pdata *pd, shmptr_of(struct window) win, scrcoord width, scrcoord height)
{
	struct window *pwin;

	pwin = fromshmptr(struct window, *pd, win);
	pwin->elements = SHMNULL;
	pwin->active_element = SHMNULL;

	if(init_screen(pd, toshmptr(*pd, &pwin->scr), width, height) != STUI_OK) return STUI_ERR;
	pwin = fromshmptr(struct window, *pd, win);

	init_input_ctl(&pwin->input_list);

	return STUI_OK;
}

void
free_window(struct shm_allocator_pdata *pd, struct window win)
{
	free_element_tree(pd, &win.elements);

	free_screen(pd, win.scr);

	free_input_ctl(pd, toshmptr(*pd, &win.input_list));
}

int
resize_window(struct shm_allocator_pdata *pd, shmptr_of(struct window) win, scrcoord new_width, scrcoord new_height)
{
	struct window *pwin;

	pwin = fromshmptr(struct window, *pd, win);
	return resize_screen(pd, toshmptr(*pd, &pwin->scr), new_width, new_height);
}

