#ifndef INPUTCTL_H
#define INPUTCTL_H

#include "util.h"

enum input_type {
	IT_KEY,
	IT_SPECIALKEY,
	IT_NONE = -1,
};
typedef uint8_t input_modifiers;
enum input_modifier {
	IM_SHIFT = 1,
	IM_CONTROL = 2,
	IM_META = 4,
	IM_SUPER = 8
};
struct input_evt {
	enum input_type type;
	union {
		struct {
			char raw;
			char parsed;
			input_modifiers mods;
		} key;
		struct {
			char raw[16];
			int key;
			input_modifiers mods;
		} special;
	} data;
};

struct input_ctl {
	struct input_evt *buf;
	data_len len, cap;
	data_len begin, end;
};

void init_input_ctl(struct input_ctl *ic);
void free_input_ctl(struct input_ctl *ic);
int  add_input_ctl (struct input_ctl *ic, struct input_evt evt);
struct input_evt get_input_ctl(struct input_ctl *ic);

#endif

