#ifndef CHAR_CELL_H
#define CHAR_CELL_H

#include <stdint.h>

enum color_type {
	CLR_USR,
	CLR_256,
	CLR_RGB,
	CLR_DEFAULT,
	CLR_UNSET,
};
struct color {
	enum color_type type;
	union {
		uint8_t usr;
		uint8_t c256;
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
		} rgb;
	} data;
};
typedef uint8_t attributes;
enum attribute {
	ATT_BOLD = 1,
	ATT_FAINT = 2,
	ATT_ITALIC = 4,
	ATT_UNDERLINE = 8,
	ATT_BLINKING = 16,
	ATT_INVERSE = 32,
	ATT_HIDDEN = 64,
	ATT_STRIKETHROUGH = 128,
};
struct char_cell {
	struct color fg, bg;
	attributes attr;
	char c;
};

int  color_eq(struct color a, struct color b);

#endif

