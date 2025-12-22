#include "inputctl.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

#define CB_ALLOC_GRAN 256

static int resize_input_ctl(struct input_ctl *ic, data_len new_size);

void
init_input_ctl(struct input_ctl *ic)
{
	ic->buf = NULL;
	ic->len = ic->cap = 0;
	ic->begin = 0;
	ic->end = -1;
}

void
free_input_ctl(struct input_ctl *ic)
{
	if(ic->buf) free(ic->buf);
	init_input_ctl(ic);
}

int
add_input_ctl(struct input_ctl *ic, struct input_evt evt)
{
	if(ic->len == ic->cap) {
		if(resize_input_ctl(ic, ic->cap + CB_ALLOC_GRAN) != STUI_OK) return STUI_ERR;
	}

	ic->end = (ic->end + 1) % ic->cap;
	ic->buf[ic->end] = evt;
	++(ic->len);

	return STUI_OK;
}

struct input_evt
get_input_ctl(struct input_ctl *ic)
{
	data_len prev_begin;

	if(ic->len == 0) {
		return (struct input_evt){.type=IT_NONE};
	}

	prev_begin = ic->begin;
	ic->begin = (ic->begin + 1) % ic->cap;
	--(ic->len);

	return ic->buf[prev_begin];
}

static int
resize_input_ctl(struct input_ctl *ic, data_len new_size)
{
	struct input_evt *new_buf;
	data_len split;
	
	new_buf = malloc(new_size * sizeof(struct input_evt));
	if(! new_buf) {
		return STUI_ERR;
	}

	if(ic->begin > ic->end) {
		split = ic->cap - ic->begin + 1;
		memcpy(new_buf, ic->buf + ic->begin, split * sizeof(struct input_evt));
		memcpy(new_buf + split, ic->buf, (ic->end + 1) * sizeof(struct input_evt));
	} else {
		memcpy(new_buf, ic->buf + ic->begin, (ic->end - ic->begin + 1) * sizeof(struct input_evt));
	}

	ic->begin = 0;
	ic->end = ic->len - 1;
	ic->cap = new_size;
	free(ic->buf);
	ic->buf = new_buf;
	
	return STUI_OK;
}

