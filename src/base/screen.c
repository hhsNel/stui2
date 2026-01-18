#include "base/screen.h"

#include <stdlib.h>
#include <string.h>

int
init_screen(struct shm_allocator_pdata *pd, shmptr_of(struct screen) scr, scrcoord width, scrcoord height)
{
	scrcoord i, j;
	struct char_cell *pccs;
	struct screen *pscr;
	shmptr_of(struct char_cell) ccs;

	shm_access(pd);

	pscr = fromshmptr(struct screen, *pd, scr);
	pscr->width = width;
	pscr->height = height;

	ccs = shm_alloc(pd, width * height * sizeof(struct char_cell));
	pscr = fromshmptr(struct screen, *pd, scr);
	if(ccs == SHMNULL) {
		shm_leave(pd);
		return STUI_ERR;
	}
	pscr->ccs = ccs;
	pccs = fromshmptr(struct char_cell, *pd, ccs);
	memset(pccs, 0, width * height * sizeof(struct char_cell));

	for(i = 0; i < width; ++i) {
		for(j = 0; j < height; ++j) {
			set_cell_screen(*pd, *pscr, (struct char_cell){.fg={.type=CLR_DEFAULT},.bg={.type=CLR_DEFAULT}}, i, j);
		}
	}

	shm_leave(pd);
	return STUI_OK;
}

void
free_screen(struct shm_allocator_pdata *pd, struct screen scr)
{
	if(scr.ccs != SHMNULL) shm_free(pd, scr.ccs);
}

int
resize_screen(struct shm_allocator_pdata *pd, shmptr_of(struct screen) scr, scrcoord new_width, scrcoord new_height)
{
	struct screen *pscr;

	shm_access(pd);

	pscr = fromshmptr(struct screen, *pd, scr);
	free_screen(pd, *pscr);

	if(init_screen(pd, scr, new_width, new_height) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}

	shm_leave(pd);
	return STUI_OK;
}

int
set_cell_screen(struct shm_allocator_pdata pd, struct screen scr, struct char_cell cc, scrcoord x, scrcoord y)
{
	struct char_cell *pccs;

	if(x >= scr.width || y >= scr.height || x < 0 || y < 0) {
		return STUI_ERR;
	}
	pccs = fromshmptr(struct char_cell, pd, scr.ccs);
	pccs[x + scr.width*y] = cc;
	return STUI_OK;
}

shmptr_of(struct char_cell)
cell_at_screen(struct shm_allocator_pdata pd, struct screen scr, scrcoord x, scrcoord y)
{
	struct char_cell *pccs;
	struct char_cell *pcell;

	if(x < 0 || y < 0 || x >= scr.width || y >= scr.height) {
		return SHMNULL;
	}

	pccs = fromshmptr(struct char_cell, pd, scr.ccs);
	pcell = pccs + y*scr.width + x;
	return toshmptr(pd, pcell);
}

int
copy_screen(struct shm_allocator_pdata *pd, shmptr_of(struct screen) scr, struct screen src)
{
	struct char_cell *pccs_scr;
	struct char_cell *pccs_src;
	scrcoord i;
	struct screen *pscr;

	shm_access(pd);

	if(resize_screen(pd, scr, src.width, src.height) != STUI_OK) {
		return STUI_ERR;
	}

	pscr = fromshmptr(struct screen, *pd, scr);
	pccs_scr = fromshmptr(struct char_cell, *pd, pscr->ccs);
	pccs_src = fromshmptr(struct char_cell, *pd, src.ccs);
	for(i = 0; i < src.width * src.height; ++i) {
		pccs_scr[i] = pccs_src[i];
	}

	shm_leave(pd);
	return STUI_OK;
}

