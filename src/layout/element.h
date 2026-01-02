#ifndef ELEMENTS_H
#define ELEMENTS_H

#include "base/screen.h"

#define RESIZABLE_COORD_BASE 16
struct coord {
	scrcoord fixed;
	uint8_t resizable;
};
struct rect {
	struct coord x, y;
	struct coord width, height;
};
enum element_type {
	ELEMENT_CUSTOM, /* TODO */
};
enum element_flag {
	ASDF, /* TODO */
};
typedef uint16_t element_flags;
typedef uint16_t element_z_index;
struct element {
	enum element_type type;
	struct rect pos;
	element_flags flags;
	element_z_index z_index;
	void *data;
};
typedef struct element *stui_element;

struct window;

#endif

