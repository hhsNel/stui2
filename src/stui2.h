#ifndef STUI2_H
#define STUI2_H

#include <stdint.h>

#define STUI_OK  0
#define STUI_ERR 1

#define STUI_INVH 0

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

typedef uint32_t stui2_element;
typedef uint32_t stui2_window;
typedef uint32_t stui2_insertable;

struct stui2;

typedef uint16_t element_z_index;

enum stui2_element_type {
	ELEMENT_LABEL,
#define ELEMENT_LABEL ELEMENT_LABEL
};

#define RESIZABLE_COORD_BASE 32
typedef int16_t scrcoord;
struct coord {
	scrcoord fixed;
	uint8_t resizable;
};
struct rect {
	struct coord x, y;
	struct coord width, height;
};

struct stui2     *init_stui2             ();
int              exit_stui2              (struct stui2 *);
stui2_window     stui2_main_window       (struct stui2 *);
stui2_insertable stui2_win_get_insertable(struct stui2 *, stui2_window);
stui2_element    stui2_create_element    (struct stui2 *, stui2_insertable, element_z_index, enum stui2_element_type, ...);
int              stui2_free_element      (struct stui2 *, stui2_element);
void             stui2_set_position      (struct stui2 *, stui2_element, struct rect);
int              stui2_render            (struct stui2 *, stui2_window);
int              stui2_flush             (struct stui2 *);

#ifdef STUI2_GLOBAL
struct stui2 *global_stui2;
#define       ginit_stui2()                   (global_stui2 = init_stui2())
#define       gexit_stui2()                   (exit_stui2(global_stui2))
#define       gmain_window()                  (stui2_main_window(global_stui2))
#define       gwin_get_insertable(WIN)        (stui2_win_get_insertable(global_stui2, WIN))
#define       gcreate_element(INS,Z,TYPE,...) (stui2_create_element(global_stui2,INS,Z,TYPE,__VA_ARGS__))
#define       gfree_element(EL)               (stui2_free_element(global_stui2, EL))
#define       gset_position(EL,RECT)          (stui2_set_position(global_stui2,EL,RECT))
#define       grender(WIN)                    (stui2_render(global_stui2,WIN))
#define       gflush()                        (stui2_flush(global_stui2))
#endif

#endif

