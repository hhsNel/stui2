#include "screen.h"

#include <stdlib.h>
#include <string.h>

int
color_eq (struct color a, struct color b)
{
	if(a.type != b.type) {
		return 0;
	}
	switch(a.type) {
	case CLR_USR:
		return a.data.usr == b.data.usr;
	case CLR_256:
		return a.data.c256 == b.data.c256;
	case CLR_RGB:
		return a.data.rgb.r == b.data.rgb.r &&
		       a.data.rgb.g == b.data.rgb.g &&
		       a.data.rgb.b == b.data.rgb.b;
	default:
		return 1;
	}
}

int
init_screen(struct char_cell ***scr, scrcoord width, scrcoord height)
{
	scrcoord i, j;

	*scr = malloc(width * sizeof(**scr));
	if(!*scr) {
		return STUI_ERR;
	}
	memset(*scr, 0, width * sizeof(**scr));

	for(i = 0; i < width; ++i) {
		(*scr)[i] = malloc(height * sizeof(***scr));
		if(! (*scr)[i]) {
			return STUI_ERR;
		}

		for(j = 0; j < height; ++j) {
			(*scr)[i][j].fg = (*scr)[i][j].bg = (struct color){.type=CLR_DEFAULT};
		}
	}

	return STUI_OK;
}

void
free_screen(struct char_cell **scr, scrcoord width, scrcoord height)
{
	scrcoord i;

	for(i = 0; i < width; ++i) {
		if(scr[i]) {
			free(scr[i]);
			scr[i] = NULL;
		}
	}

	free(scr);
}

int
resize_screen(struct char_cell ***scr, scrcoord width, scrcoord height, scrcoord new_width, scrcoord new_height)
{
	free_screen(*scr, width, height);
	return init_screen(scr, new_width, new_height);
}

int
set_cell_screen(struct char_cell  **scr, scrcoord width, scrcoord height, struct char_cell cc, scrcoord x, scrcoord y)
{
	if(x >= width || y >= height) {
		return STUI_ERR;
	}

	scr[x][y] = cc;
	return STUI_OK;
}

int
copy_screen(struct char_cell ***scr, struct char_cell **src, scrcoord prev_width, scrcoord new_width, scrcoord prev_height, scrcoord new_height)
{
	scrcoord i, j;

	if(resize_screen(scr, prev_width, prev_height, new_width, new_height) != STUI_OK) {
		return STUI_ERR;
	}

	for(i = 0; i < new_width; ++i) {
		for(j = 0; j < new_height; ++j) {
			(*scr)[i][j] = src[i][j];
		}
	}

	return STUI_OK;
}

