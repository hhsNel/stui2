#include "label.h"

#include <stdarg.h>
#include <string.h>

#include "util.h"
#include "base/charcell.h"
#include "shm/allocator.h"

struct label_data {
	struct char_cell style;
	shmptr_of(char) string;
};

int
init_element_label(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, va_list args)
{
	struct element *pel;
	struct label_data *pdata;
	char *pstring;
	struct char_cell style;
	char *string;

	style = va_arg(args, struct char_cell);
	string = va_arg(args, char *);
	pel = fromshmptr(struct element, *pd, el);

	pel->data.type_data = shm_alloc(pd, sizeof(struct label_data));
	pel = fromshmptr(struct element, *pd, el);
	if(pel->data.type_data == SHMNULL) {
		return STUI_ERR;
	}
	pdata = fromshmptr(struct label_data, *pd, pel->data.type_data);
	pdata->style = style;
	pdata->string = shm_alloc(pd, strlen(string) + 1);
	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct label_data, *pd, pel->data.type_data);
	if(pdata->string == SHMNULL) {
		return STUI_ERR;
	}
	pstring = fromshmptr(char, *pd, pdata->string);
	strcpy(pstring, string);

	return STUI_OK;
}

void
free_element_label(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	struct element *pel;
	struct label_data *pdata;

	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct label_data, *pd, pel->data.type_data);
	shm_free(pd, pdata->string);
	pel = fromshmptr(struct element, *pd, el);
	shm_free(pd, pel->data.type_data);
}

int
element_draw_label(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	struct element *pel;
	scrcoord i, j, x, y;
	struct label_data *pdata;
	char *iter;
	struct screen *output;

	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct label_data, *pd, pel->data.type_data);
	iter = fromshmptr(char, *pd, pdata->string);
	output = fromshmptr(struct screen, *pd, pel->render_output);
	for(i = 0; i < pel->scr_pos.height; ++i) {
		y = pel->scr_pos.y + i;
		for(j = 0; j < pel->scr_pos.width; ++j, ++iter) {
			switch(*iter) {
			case '\0':
				return STUI_OK;
			case '\n':
				++iter;
				goto line_end;
			}
			x = pel->scr_pos.x + j;
			pdata->style.c = *iter;
			if(set_cell_screen(*pd, *output, pdata->style, x, y) != STUI_OK) {
				return STUI_ERR;
			}
		}
line_end:
	}
	return STUI_OK;
}

int
element_resize_label(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	return STUI_OK;
}

