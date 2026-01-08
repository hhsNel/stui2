#include "base/screen.h"

#include <stdlib.h>
#include <string.h>

int
init_screen(struct shm_allocator_pdata *pd, struct screen *scr, scrcoord width, scrcoord height)
{
	scrcoord i, j;
	struct char_cell *pccs;

	shm_access(pd);

	scr->ccs = shm_alloc(pd, width * height * sizeof(struct char_cell));
	if(scr->ccs == SHMNULL) {
		return STUI_ERR;
	}
	pccs = fromshmptr(struct char_cell, *pd, scr->ccs);
	memset(pccs, 0, width * height * sizeof(struct char_cell));

	for(i = 0; i < width; ++i) {
		for(j = 0; j < height; ++j) {
			set_cell_screen(*pd, *scr, (struct char_cell){.fg={.type=CLR_DEFAULT},.bg={.type=CLR_DEFAULT}}, i, j);
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
resize_screen(struct shm_allocator_pdata *pd, struct screen *scr, scrcoord new_width, scrcoord new_height)
{
	free_screen(pd, *scr);
	return init_screen(pd, scr, new_width, new_height);
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
copy_screen(struct shm_allocator_pdata *pd, struct screen *scr, struct screen src)
{
	struct char_cell *pccs_scr;
	struct char_cell *pccs_src;
	scrcoord i;

	shm_access(pd);

	if(resize_screen(pd, scr, src.width, src.height) != STUI_OK) {
		return STUI_ERR;
	}

	pccs_scr = fromshmptr(struct char_cell, *pd, scr->ccs);
	pccs_src = fromshmptr(struct char_cell, *pd, src.ccs);
	for(i = 0; i < src.width * src.height; ++i) {
		pccs_scr[i] = pccs_src[i];
	}

	shm_leave(pd);
	return STUI_OK;
}

