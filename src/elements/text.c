#include "text.h"

#include <stdarg.h>
#include <string.h>

#include "util.h"
#include "base/charcell.h"
#include "shm/allocator.h"

#define LINES_ALLOC_GRAN 8

struct text_chunk {
	struct char_cell style;
	shmptr_of(char) string;
	shmptr_of(struct text_chunk) next;
};

struct text_data {
	enum stui2_element_text_mode mode;
	shmptr_of(shmptr_of(struct text_chunk)) lines;
	scrcoord num_lines, cap_lines;
	scrcoord cur_x;
	scrcoord cur_y;

	scrcoord line_idx;
};

static int draw_text_chunk(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, shmptr_of(struct text_chunk) tc, unsigned int line);
static unsigned int next_break(struct shm_allocator_pdata pd, struct element *el, char *string);
static shmptr_of(struct text_chunk) new_chunk(struct shm_allocator_pdata *pd, shmptr_of(struct text_data) td);
static int fill_chunk(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, shmptr_of(struct text_chunk) chunk, struct char_cell cc, char **str);

int
init_element_text(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, va_list args)
{
	enum stui2_element_text_mode mode;
	struct element *pel;
	struct text_data *pdata;

	mode = va_arg(args, enum stui2_element_text_mode);

	pel = fromshmptr(struct element, *pd, el);
	pel->data.type_data = shm_alloc(pd, sizeof(struct text_data));
	pel = fromshmptr(struct element, *pd, el);
	if(pel->data.type_data == SHMNULL) {
		return STUI_ERR;
	}
	pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);

	pdata->mode = mode;
	pdata->lines = SHMNULL;
	pdata->num_lines = pdata->cap_lines = 0;
	pdata->cur_x = pdata->cur_y = 0;

	return STUI_OK;
}

void
free_element_text(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	struct element *pel;
	struct text_data *pdata;
	shmptr_of(struct text_chunk) *plines;
	unsigned int i;
	shmptr_of(struct text_chunk) iter;
	struct text_chunk *piter;
	shmptr_of(struct text_chunk) next;

	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);
	plines = fromshmptr(shmptr_of(struct text_chunk), *pd, pdata->lines);

	for(i = 0; i < (unsigned int)pdata->cap_lines; ++i) {
		iter = plines[i];
		while(iter != SHMNULL) {
			piter = fromshmptr(struct text_chunk, *pd, iter);
			next = piter->next;

			shm_free(pd, piter->string);
			shm_free(pd, iter);
			pel = fromshmptr(struct element, *pd, el);
			pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);
			plines = fromshmptr(shmptr_of(struct text_chunk), *pd, pdata->lines);

			iter = next;
		}
	}

	shm_free(pd, pdata->lines);
}

int
element_draw_text(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	struct element *pel;
	struct text_data *pdata;
	shmptr_of(struct text_chunk) *plines;
	unsigned int i;
	shmptr_of(struct text_chunk) iter;
	struct text_chunk *piter;

	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);
	plines = fromshmptr(shmptr_of(struct text_chunk), *pd, pdata->lines);

	for(i = 0; i < (unsigned int)pdata->num_lines; ++i) {
		pdata->line_idx = 0;

		iter = plines[i];
		while(iter != SHMNULL) {
			piter = fromshmptr(struct text_chunk, *pd, iter);

			if(draw_text_chunk(pd, el, iter, i) != STUI_OK) {
				return STUI_ERR;
			}
			pel = fromshmptr(struct element, *pd, el);
			pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);
			plines = fromshmptr(shmptr_of(struct text_chunk), *pd, pdata->lines);
			piter = fromshmptr(struct text_chunk, *pd, iter);

			iter = piter->next;
		}

		for(; pdata->line_idx < pel->scr_pos.width; ++pdata->line_idx) {
			if(set_cell_screen(*pd, *fromshmptr(struct screen, *pd, pel->render_output), (struct char_cell){.c=' ',.fg={.type=CLR_DEFAULT},.bg={.type=CLR_DEFAULT}}, pel->scr_pos.x + pdata->line_idx, pel->scr_pos.y + i) != STUI_OK) {
				return STUI_ERR;
			}
		}
	}

	return STUI_OK;
}

int
element_resize_text(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	/* TODO */
	return STUI_OK;
}

int
text_append(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, struct char_cell cc, char *str)
{
	struct element *pel;
	struct text_data *pdata;
	shmptr_of(struct text_chunk) chunk;

	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);

	while(*str) {
		chunk = new_chunk(pd, el);
		if(chunk == SHMNULL) {
			return STUI_ERR;
		}
		fill_chunk(pd, el, chunk, cc, &str);
		pel = fromshmptr(struct element, *pd, el);
		pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);

		if(pdata->cur_x == pel->scr_pos.width || *str != '\0') {
			pdata->cur_x = pdata->line_idx = 0;
			++pdata->cur_y;
			if(*str == '\n') {
				++str;
			}
		}
	}

	return STUI_OK;
}

static int
draw_text_chunk(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, shmptr_of(struct text_chunk) tc, unsigned int i)
{
	struct element *pel;
	struct text_data *pdata;
	struct text_chunk *ptc;
	char *pstring;

	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);
	ptc = fromshmptr(struct text_chunk, *pd, tc);
	pstring = fromshmptr(char, *pd, ptc->string);

	for(; *pstring; ++pstring, ++pdata->line_idx) {
		ptc->style.c = *pstring;

		if(set_cell_screen(*pd, *fromshmptr(struct screen, *pd, pel->render_output), ptc->style, pel->scr_pos.x + pdata->line_idx, pel->scr_pos.y + i) != STUI_OK) {
			return STUI_ERR;
		}
	}

	return STUI_OK;
}

static unsigned int
next_break(struct shm_allocator_pdata pd, struct element *el, char *string)
{
	unsigned int i;
	struct text_data *pdata;

	pdata = fromshmptr(struct text_data, pd, el->data.type_data);

	for(i = 0; string[i]; ++i) {
		if((scrcoord)i + pdata->cur_x == el->scr_pos.width) {
			return i;
		}
		if(string[i] == '\n') {
			return i;
		}
	}

	return i;
}

static shmptr_of(struct text_chunk)
new_chunk(struct shm_allocator_pdata *pd, shmptr_of(struct element) el)
{
	struct element *pel;
	struct text_data *pdata;
	shmptr_of(shmptr_of(struct text_chunk)) lines;
	shmptr_of(struct text_chunk) *plines;
	shmptr_of(struct text_chunk) chunk, next_chunk;
	struct text_chunk *pchunk;

	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);
	plines = fromshmptr(shmptr_of(struct text_chunk), *pd, pdata->lines);

	if(pdata->num_lines > pel->scr_pos.height) {
		if(pdata->mode & EL_TEXT_SCROLL) {
			shm_free(pd, plines[0]);
			pel = fromshmptr(struct element, *pd, el);
			pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);
			plines = fromshmptr(shmptr_of(struct text_chunk), *pd, pdata->lines);

			memmove(plines, plines + 1, (pdata->num_lines - 1) * sizeof(shmptr_of(struct text_chunk)));
			plines[pdata->num_lines - 1] = SHMNULL;
		} else {
			pdata->cur_y = 0;
		}
	}

	if(pdata->cap_lines <= pdata->cur_y) {
		pdata->cap_lines += LINES_ALLOC_GRAN;
		lines = shm_realloc(pd, pdata->lines, pdata->cap_lines * sizeof(shmptr_of(struct text_chunk)));
		pel = fromshmptr(struct element, *pd, el);
		pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);
		pdata->lines = lines;
		if(pdata->lines == SHMNULL) {
			return SHMNULL;
		}
		plines = fromshmptr(shmptr_of(struct text_chunk), *pd, pdata->lines);
	}

	if(pdata->cur_y >= pdata->num_lines) {
		pdata->num_lines = pdata->cur_y + 1;
	}

	chunk = plines[pdata->cur_y];
	if(chunk != SHMNULL) {
		pchunk = fromshmptr(struct text_chunk, *pd, chunk);
		while(pchunk->next != SHMNULL) {
			chunk = pchunk->next;
			pchunk = fromshmptr(struct text_chunk, *pd, chunk);
		}

		next_chunk = shm_alloc(pd, sizeof(struct text_chunk));
		pchunk = fromshmptr(struct text_chunk, *pd, chunk);
		chunk = pchunk->next = next_chunk;
	} else {
		chunk = plines[pdata->cur_y] = shm_alloc(pd, sizeof(struct text_chunk));
	}

	return chunk;
}

static int
fill_chunk(struct shm_allocator_pdata *pd, shmptr_of(struct element) el, shmptr_of(struct text_chunk) chunk, struct char_cell cc, char **str)
{
	struct element *pel;
	struct text_data *pdata;
	struct text_chunk *pchunk;
	unsigned int chunk_len;
	char *pstring;

	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);
	pchunk = fromshmptr(struct text_chunk, *pd, chunk);
	pstring = fromshmptr(char, *pd, pchunk->string);

	chunk_len = next_break(*pd, pel, *str);

	pchunk->style = cc;
	pchunk->next = SHMNULL;
	pchunk->string = shm_alloc(pd, chunk_len + 1);
	pchunk = fromshmptr(struct text_chunk, *pd, chunk);
	if(pchunk->string == SHMNULL) {
		return STUI_ERR;
	}
	pel = fromshmptr(struct element, *pd, el);
	pdata = fromshmptr(struct text_data, *pd, pel->data.type_data);
	pstring = fromshmptr(char, *pd, pchunk->string);

	memcpy(pstring, *str, chunk_len);
	pstring[chunk_len] = '\0';
	*str += chunk_len;
	pdata->cur_x += chunk_len;

	return STUI_OK;
}

