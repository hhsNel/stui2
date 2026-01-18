#ifndef SCREEN_H
#define SCREEN_H

#include "util.h"
#include "base/charcell.h"
#include "shm/allocator.h"

#include <stdint.h>

typedef int16_t scrcoord;
struct screen {
	scrcoord width;
	scrcoord height;
	shmptr_of(struct char_cell) ccs; /* row-major */
};

int  init_screen    (struct shm_allocator_pdata *pd, shmptr_of(struct screen) scr, scrcoord width, scrcoord height);
void free_screen    (struct shm_allocator_pdata *pd, struct screen scr);
int  resize_screen  (struct shm_allocator_pdata *pd, shmptr_of(struct screen) scr, scrcoord new_width, scrcoord new_height);
int  set_cell_screen(struct shm_allocator_pdata pd, struct screen scr, struct char_cell cc, scrcoord x, scrcoord y);
shmptr_of(struct char_cell) cell_at_screen(struct shm_allocator_pdata pd, struct screen scr, scrcoord x, scrcoord y);
int  copy_screen    (struct shm_allocator_pdata *pd, shmptr_of(struct screen) scr, struct screen src);

#endif

