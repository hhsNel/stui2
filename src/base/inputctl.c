#include "base/inputctl.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

#define CB_ALLOC_GRAN 256

static int resize_input_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct input_ctl) ic, data_len new_size);

void
init_input_ctl(struct input_ctl *ic)
{
	ic->buf = SHMNULL;
	ic->len = ic->cap = 0;
	ic->begin = 0;
	ic->end = -1;
}

void
free_input_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct input_ctl) ic)
{
	struct input_ctl *pic;

	shm_access(pd);

	pic = fromshmptr(struct input_ctl, *pd, ic);

	if(pic->buf != SHMNULL) shm_free(pd, pic->buf);
	init_input_ctl(pic);

	shm_leave(pd);
}

int
add_input_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct input_ctl) ic, struct input_evt evt)
{
	struct input_evt *pbuf;
	struct input_ctl *pic;

	shm_access(pd);

	pic = fromshmptr(struct input_ctl, *pd, ic);
	if(pic->len == pic->cap) {
		if(resize_input_ctl(pd, ic, pic->cap + CB_ALLOC_GRAN) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
		pic = fromshmptr(struct input_ctl, *pd, ic);
	}

	pbuf = fromshmptr(struct input_evt, *pd, pic->buf);
	pic->end = (pic->end + 1) % pic->cap;
	pbuf[pic->end] = evt;
	++(pic->len);

	shm_leave(pd);
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
resize_input_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct input_ctl) ic, data_len new_size)
{
	shmptr_of(struct input_evt) new_buf;
	struct input_evt *pnew_buf, *pbuf;
	data_len split;
	struct input_ctl *pic;

	shm_access(pd);

	new_buf = shm_alloc(pd, new_size * sizeof(struct input_evt));
	if(new_buf == SHMNULL) {
		return STUI_ERR;
	}

	pic = fromshmptr(struct input_ctl, *pd, ic);
	pbuf = fromshmptr(struct input_evt, *pd, pic->buf);
	pnew_buf = fromshmptr(struct input_evt, *pd, new_buf);
	if(pic->begin > pic->end) {
		split = pic->cap - pic->begin + 1;
		memcpy(pnew_buf, pbuf + pic->begin, split * sizeof(struct input_evt));
		memcpy(pnew_buf + split, pbuf, (pic->end + 1) * sizeof(struct input_evt));
	} else {
		memcpy(pnew_buf, pbuf + pic->begin, (pic->end - pic->begin + 1) * sizeof(struct input_evt));
	}

	pic->begin = 0;
	pic->end = pic->len - 1;
	pic->cap = new_size;
	if(pic->buf != SHMNULL) {
		shm_free(pd, pic->buf);
	}
	pic->buf = new_buf;
	
	shm_leave(pd);
	return STUI_OK;
}

