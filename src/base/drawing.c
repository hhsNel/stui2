#include "base/drawing.h"
#include "base/screen.h"

int
draw_line_screen(struct shm_allocator_pdata pd, struct screen scr, scrcoord x0, scrcoord y0, scrcoord x1, scrcoord y1, struct char_cell fill)
{
	scrcoord delta_x, delta_y;
	scrcoord sign_x, sign_y;
	scrcoord error;

	sign_x = (x1 > x0) ? 1 : -1;
	sign_y = (y1 > y0) ? 1 : -1;

	delta_x = (x1 - x0) * sign_x;
	delta_y = (y1 - y0) * sign_y;

	error = delta_x - delta_y;

	while(x0 != x1 || y0 != y1) {
		if(set_cell_screen(pd, scr, fill, x0, y0) != STUI_OK) return STUI_ERR;

		if(2 * error > -delta_y) {
			error -= delta_y;
			x0 += sign_x;
		}
		if(2 * error < delta_x) {
			error += delta_x;
			y0 += sign_y;
		}
	}
	if(set_cell_screen(pd, scr, fill, x0, y0) != STUI_OK) return STUI_ERR;

	return STUI_OK;
}

int
draw_line_dblbuf(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord x0, scrcoord y0, scrcoord x1, scrcoord y1, struct char_cell fill)
{
	return draw_line_screen(pd, db->cur_scr, x0, y0, x1, y1, fill);
}

int
fill_rect_screen(struct shm_allocator_pdata pd, struct screen scr, scrcoord x, scrcoord y, scrcoord w, scrcoord h, struct char_cell fill)
{
	scrcoord i, j;

	for(i = 0; i < w; ++i) {
		for(j = 0; j < h; ++j) {
			if(set_cell_screen(pd, scr, fill, x+i, y+j) != STUI_OK) return STUI_ERR;
		}
	}

	return STUI_OK;
}

int
fill_rect_dblbuf(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord x, scrcoord y, scrcoord w, scrcoord h, struct char_cell fill)
{
	return fill_rect_screen(pd, db->cur_scr, x, y, w, h, fill);
}

int
write_string_screen(struct shm_allocator_pdata pd, struct screen scr, scrcoord x, scrcoord y, scrcoord w, scrcoord h, char *str, struct char_cell ex)
{
	scrcoord i, j;

	for(i = 0; i < w && *str; ++i) {
		for(j = 0; j < h && *str; ++j) {
			ex.c = *str;
			if(set_cell_screen(pd, scr, ex, x+i, y+j) != STUI_OK) return STUI_ERR;
			++str;
		}
	}

	return STUI_OK;
}

int
write_string_dblbuf(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord x, scrcoord y, scrcoord w, scrcoord h, char *str, struct char_cell ex)
{
	return write_string_screen(pd, db->cur_scr, x, y, w, h, str, ex);
}

