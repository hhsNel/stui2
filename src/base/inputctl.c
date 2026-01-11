#include "base/inputctl.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

#define CB_ALLOC_GRAN 256

static int resize_input_ctl(struct shm_allocator_pdata *pd, struct input_ctl *ic, data_len new_size);

void
init_input_ctl(struct input_ctl *ic)
{
	ic->buf = SHMNULL;
	ic->len = ic->cap = 0;
	ic->begin = 0;
	ic->end = -1;
}

void
free_input_ctl(struct shm_allocator_pdata *pd, struct input_ctl *ic)
{
	if(ic->buf != SHMNULL) shm_free(pd,ic->buf);
	init_input_ctl(ic);
}

int
add_input_ctl(struct shm_allocator_pdata *pd, struct input_ctl *ic, struct input_evt evt)
{
	struct input_evt *pbuf;

	if(ic->len == ic->cap) {
		if(resize_input_ctl(pd, ic, ic->cap + CB_ALLOC_GRAN) != STUI_OK) return STUI_ERR;
	}

	pbuf = fromshmptr(struct input_evt, *pd, ic->buf);
	ic->end = (ic->end + 1) % ic->cap;
	pbuf[ic->end] = evt;
	++(ic->len);

	return STUI_OK;
}

struct input_evt
get_input_ctl(struct shm_allocator_pdata pd, struct input_ctl *ic)
{
	data_len prev_begin;
	struct input_evt *pbuf;

	if(ic->len == 0) {
		return (struct input_evt){.type=IT_NONE};
	}

	prev_begin = ic->begin;
	ic->begin = (ic->begin + 1) % ic->cap;
	--(ic->len);

	pbuf = fromshmptr(struct input_evt, pd, ic->buf);
	return pbuf[prev_begin];
}

static int
resize_input_ctl(struct shm_allocator_pdata *pd, struct input_ctl *ic, data_len new_size)
{
	shmptr_of(struct input_evt) new_buf;
	struct input_evt *pnew_buf, *pbuf;
	data_len split;
	
	new_buf = shm_alloc(pd, new_size * sizeof(struct input_evt));
	if(new_buf == SHMNULL) {
		return STUI_ERR;
	}

	pbuf = fromshmptr(struct input_evt, *pd, ic->buf);
	pnew_buf = fromshmptr(struct input_evt, *pd, new_buf);
	if(ic->begin > ic->end) {
		split = ic->cap - ic->begin + 1;
		memcpy(pnew_buf, pbuf + ic->begin, split * sizeof(struct input_evt));
		memcpy(pnew_buf + split, pbuf, (ic->end + 1) * sizeof(struct input_evt));
	} else {
		memcpy(pnew_buf, pbuf + ic->begin, (ic->end - ic->begin + 1) * sizeof(struct input_evt));
	}

	ic->begin = 0;
	ic->end = ic->len - 1;
	ic->cap = new_size;
	if(ic->buf != SHMNULL) {
		shm_free(pd, ic->buf);
	}
	ic->buf = new_buf;
	
	return STUI_OK;
}

