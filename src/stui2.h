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
enum special_key {
	SK_BACKSPACE,
	SK_TAB,
	SK_RETURN,
	SK_ESCAPE,
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
			char raw[8];
			enum special_key parsed;
			input_modifiers mods;
		} special;
	} data;
};

enum stui2_element_text_mode {
	EL_TEXT_NOSCROLL = 0,
	EL_TEXT_SCROLL = 1,
};

typedef uint32_t stui2_element;
typedef uint32_t stui2_window;
typedef uint32_t stui2_insertable;

struct stui2;

typedef uint16_t element_z_index;

enum stui2_element_type {
	ELEMENT_LABEL,
#define ELEMENT_LABEL ELEMENT_LABEL
	ELEMENT_TEXT,
#define ELEMENT_TEXT ELEMENT_TEXT
	ELEMENT_DRAW,
#define ELEMENT_DRAW ELEMENT_DRAW
};

#define RESIZABLE_COORD_BASE 32
typedef int32_t scrcoord;
struct coord {
	scrcoord fixed;
	uint8_t resizable;
};
struct rect {
	struct coord x, y;
	struct coord width, height;
};

typedef uint64_t stui2_info;
enum stui2_sub_info {
	INFO_HAS_AGFX      = 1,
	INFO_USES_AGFX     = 2,

	INFO_HAS_AGFX_FB   = 4,
	INFO_USES_AGFX_FB  = 8,
	INFO_HAS_AGFX_DRI  = 16,
	INFO_USES_AGFX_DRI = 32,
};

struct stui2     *init_stui2             ();
int              exit_stui2              (struct stui2 *);
int              stui2_is_parent         (struct stui2 *);
stui2_info       stui2_get_info          (struct stui2 *);
char const       *stui2_cur_comp_date    ();
char const       *stui2_cur_comp_time    ();
void             stui2_parent_comp_date  (struct stui2 *, char *, unsigned int);
void             stui2_parent_comp_time  (struct stui2 *, char *, unsigned int);
void             stui2_get_shm_id        (struct stui2 *, char *, unsigned int);
stui2_window     stui2_main_window       (struct stui2 *);
stui2_insertable stui2_win_get_insertable(struct stui2 *, stui2_window);
stui2_element    stui2_create_element    (struct stui2 *, stui2_insertable, element_z_index, enum stui2_element_type, ...);
int              stui2_free_element      (struct stui2 *, stui2_element);
void             stui2_set_position      (struct stui2 *, stui2_element, struct rect);
int              stui2_render            (struct stui2 *, stui2_window);
int              stui2_flush             (struct stui2 *);
int              stui2_get_input         (struct stui2 *, stui2_window, int);
struct input_evt stui2_next_event        (struct stui2 *, stui2_window);
scrcoord         stui2_insertable_width  (struct stui2 *, stui2_insertable);
scrcoord         stui2_insertable_height (struct stui2 *, stui2_insertable);

int  stui2_label_set_style(struct stui2 *, stui2_element, struct char_cell);
int  stui2_label_set_string(struct stui2 *, stui2_element, char *);
int  stui2_text_append(struct stui2 *, stui2_element, struct char_cell, char *);
int  stui2_text_printf(struct stui2 *, stui2_element, struct char_cell, char *, ...);
int stui2_draw_set(struct stui2 *, stui2_element, scrcoord, scrcoord, struct char_cell);
int stui2_draw_line(struct stui2 *, stui2_element, scrcoord, scrcoord, scrcoord, scrcoord, struct char_cell);
int stui2_draw_rect(struct stui2 *, stui2_element, scrcoord, scrcoord, scrcoord, scrcoord, struct char_cell);

#ifdef STUI2_GLOBAL
extern struct stui2 *global_stui2;
#define       ginit_stui2()                   (global_stui2 = init_stui2())
#define       gexit_stui2()                   (exit_stui2(global_stui2))
#define       gis_parent()                    (stui2_is_parent(global_stui2))
#define       gget_info()                     (stui2_get_info(global_stui2))
#define       gparent_comp_date(BUF,LEN)      (stui2_parent_comp_date(global_stui2, BUF, LEN))
#define       gparent_comp_time(BUF,LEN)      (stui2_parent_comp_time(global_stui2, BUF, LEN))
#define       gget_shm_id(BUF,LEN)            (stui2_get_shm_id(global_stui2, BUF, LEN))
#define       gmain_window()                  (stui2_main_window(global_stui2))
#define       gwin_get_insertable(WIN)        (stui2_win_get_insertable(global_stui2, WIN))
#define       gcreate_element(INS,Z,TYPE,...) (stui2_create_element(global_stui2,INS,Z,TYPE,##__VA_ARGS__))
#define       gfree_element(EL)               (stui2_free_element(global_stui2, EL))
#define       gset_position(EL,RECT)          (stui2_set_position(global_stui2,EL,RECT))
#define       grender(WIN)                    (stui2_render(global_stui2,WIN))
#define       gflush()                        (stui2_flush(global_stui2))
#define       glabel_set_style(EL,CC)         (stui2_label_set_style(global_stui2,EL,CC)
#define       glabel_set_string(EL,STR)       (stui2_label_set_string(global_stui2,EL,STR)
#define       gget_input(WIN,TIME)            (stui2_get_input(global_stui2,WIN,TIME))
#define       gnext_event(WIN)                (stui2_next_event(global_stui2,WIN))
#define       ginsertable_width(INS)          (stui2_insertable_width(global_stui2,INS))
#define       ginsertable_height(INS)         (stui2_insertable_height(global_stui2,INS))
#endif

#endif

