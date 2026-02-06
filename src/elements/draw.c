#include "draw.h"

#include <stdarg.h>
#include <string.h>

#include "util.h"
#include "base/charcell.h"
#include "shm/allocator.h"

struct draw_data {
	struct char_cell background;
	shmptr_of(shmptr_of(struct char_cell)) screen;
	scrcoord width, height;
};

int
init_element_draw(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, va_list args)
{
	struct element *pel;
	struct draw_data *pdata;
	shmptr_of(struct char_cell) *pscreen;
	struct char_cell *prow;
	scrcoord i, j;
	struct char_cell background;

	background = va_arg(args, struct char_cell);
	pel = fromshmptr(struct element, *pd, el);

	/* TODO: pel can be invalidated by shm_alloc and crash, fix all instances @ next refactor */
	pel->data.type_data = shm_alloc(pd, sizeof(struct draw_data));
	pel = fromshmptr(struct element, *pd, el);
	if(pel->data.type_data == SHMNULL) {
		return STUI_ERR;
	}
	pdata = fromshmptr(struct draw_data, *pd, pel->data.type_data);

	pdata->background = background;

	pdata->screen = shm_alloc(pd, sizeof(shmptr_of(struct char_cell)) * pel->scr_pos.height);
	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct draw_data, *pd, pel->data.type_data);
	if(pdata->screen == SHMNULL) {
		return STUI_ERR;
	}
	pscreen = fromshmptr(shmptr_of(struct char_cell), *pd, pdata->screen);

	for(i = 0; i < pel->scr_pos.height; ++i) {
		pscreen[i] = shm_alloc(pd, sizeof(struct char_cell) * pel->scr_pos.width);
		pel = fromshmptr(struct element, *pd, el);
		pdata = fromshmptr(struct draw_data, *pd, pel->data.type_data);
		pscreen = fromshmptr(shmptr_of(struct char_cell), *pd, pdata->screen);
		if(pscreen[i] == SHMNULL) {
			return STUI_ERR;
		}
		prow = fromshmptr(struct char_cell, *pd, pscreen[i]);

		for(j = 0; j < pel->scr_pos.width; ++j) {
			prow[j] = background;
		}
	}

	pdata->width = pel->scr_pos.width;
	pdata->height = pel->scr_pos.height;

	return STUI_OK;
}

void
free_element_draw(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	struct element *pel;
	struct draw_data *pdata;
	shmptr_of(struct draw_data) *pscreen;
	scrcoord i;

	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct draw_data, *pd, pel->data.type_data);
	pscreen = fromshmptr(shmptr_of(struct draw_data), *pd, pdata->screen);

	for(i = 0; i < pdata->height; ++i) {
		shm_free(pd, pscreen[i]);
		pel = fromshmptr(struct element, *pd, el);
		pdata = fromshmptr(struct draw_data, *pd, pel->data.type_data);
		pscreen = fromshmptr(shmptr_of(struct draw_data), *pd, pdata->screen);
	}
	shm_free(pd, pdata->screen);
	shm_free(pd, pel->data.type_data);
}

int
element_draw_draw(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	struct element *pel;
	struct draw_data *pdata;
	shmptr_of(struct char_cell) *pscreen;
	struct char_cell *prow;
	scrcoord i, j;
	struct screen *output;

	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct draw_data, *pd, pel->data.type_data);
	pscreen = fromshmptr(shmptr_of(struct draw_data), *pd, pdata->screen);
	output = fromshmptr(struct screen, *pd, pel->render_output);

	for(i = 0; i < pdata->height; ++i) {
		prow = fromshmptr(struct char_cell, *pd, pscreen[i]);
		for(j = 0; j < pdata->width; ++j) {
			if(set_cell_screen(*pd, *output, prow[j], pel->scr_pos.x + j, pel->scr_pos.y + i) != STUI_OK) {
				return STUI_ERR;
			}
		}
	}

	return STUI_OK;
}

int
element_resize_draw(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	struct element *pel;
	struct draw_data *pdata;
	shmptr_of(struct char_cell) *pscreen;
	shmptr_of(shmptr_of(struct char_cell)) screen;
	shmptr_of(struct char_cell) row;
	scrcoord i;

	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct draw_data, *pd, pel->data.type_data);
	pscreen = fromshmptr(shmptr_of(struct draw_data), *pd, pdata->screen);

	for(i = pel->scr_pos.height; i < pdata->height; ++i) {
		shm_free(pd, pscreen[i]);
		pel = fromshmptr(struct element, *pd, el);
		pdata = fromshmptr(struct draw_data, *pd, pel->data.type_data);
		pscreen = fromshmptr(shmptr_of(struct draw_data), *pd, pdata->screen);
	}
	screen = shm_realloc(pd, pdata->screen, pel->scr_pos.height * sizeof(shmptr_of(struct char_cell)));
	if(screen == SHMNULL) {
		return STUI_ERR;
	}
	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct draw_data, *pd, pel->data.type_data);
	pdata->screen = screen;
	pscreen = fromshmptr(shmptr_of(struct draw_data), *pd, pdata->screen);
	for(i = pdata->height; i < pel->scr_pos.height; ++i) {
		pscreen[i] = SHMNULL;
	}

	for(i = 0; i < pel->scr_pos.height; ++i) {
		row = shm_realloc(pd, pscreen[i], pel->scr_pos.width * sizeof(struct char_cell));
		if(row == SHMNULL) {
			return STUI_ERR;
		}
		pel = fromshmptr(struct element, *pd, el);
		pdata = fromshmptr(struct draw_data, *pd, pel->data.type_data);
		pscreen = fromshmptr(shmptr_of(struct draw_data), *pd, pdata->screen);
		pscreen[i] = row;
	}

	pdata->width = pel->scr_pos.width;
	pdata->height = pel->scr_pos.height;

	return STUI_OK;
}

void
draw_setcc(struct shm_allocator_pdata pd, shmptr_of(struct element) el, scrcoord x, scrcoord y, struct char_cell cc)
{
	struct element *pel;
	struct draw_data *pdata;
	shmptr_of(struct char_cell) *pscreen;
	struct char_cell *prow;

	pel = fromshmptr(struct element, pd, el);
	pdata = fromshmptr(struct draw_data, pd, pel->data.type_data);
	pscreen = fromshmptr(shmptr_of(struct draw_data), pd, pdata->screen);
	prow = fromshmptr(struct char_cell, pd, pscreen[y]);

	/* TODO: validate */
	prow[x] = cc;
}

void
draw_mkline(struct shm_allocator_pdata pd, shmptr_of(struct element) el, scrcoord x0, scrcoord y0, scrcoord x1, scrcoord y1, struct char_cell fill)
{
	scrcoord delta_x, delta_y;
	scrcoord sign_x, sign_y;
	scrcoord error, e2;

	sign_x = (x1 > x0) ? 1 : -1;
	sign_y = (y1 > y0) ? 1 : -1;

	delta_x = (x1 - x0) * sign_x;
	delta_y = (y1 - y0) * sign_y;

	error = delta_x - delta_y;

	while(x0 != x1 || y0 != y1) {
		draw_setcc(pd, el, x0, y0, fill);

		e2 = 2 * error;
		if(e2 > -delta_y) {
			error -= delta_y;
			x0 += sign_x;
		}
		if(e2 < delta_x) {
			error += delta_x;
			y0 += sign_y;
		}
	}
	draw_setcc(pd, el, x0, y0, fill);
}

void
draw_rect(struct shm_allocator_pdata pd, shmptr_of(struct element) el, scrcoord x, scrcoord y, scrcoord width, scrcoord height, struct char_cell fill)
{
	scrcoord i, j;

	for(i = 0; i < width; ++i) {
		for(j = 0; j < height; ++j) {
			draw_setcc(pd, el, x + i, y + j, fill);
		}
	}
}

