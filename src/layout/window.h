#ifndef WINDOW_H
#define WINDOW_H

#include "shm/allocator.h"
#include "layout/element.h"
#include "base/screen.h"
#include "base/inputctl.h"

struct window {
	shmptr_of(struct element) active_element;
	struct stui2_insertable ins;

	struct input_ctl input_list;
};

int  init_window(struct shm_allocator_pdata *pd, shmptr_of(struct window) win, scrcoord width, scrcoord height);
void free_window(struct shm_allocator_pdata *pd, struct window *win);
int  resize_window(struct shm_allocator_pdata *pd, shmptr_of(struct window) win, scrcoord new_width, scrcoord new_height);

#endif

