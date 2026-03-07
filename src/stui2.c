#include "stui2.h"

#include "shm/shm.h"
#include "shm/allocator.h"
#include "layout/window.h"
#include "base/dblbuf.h"
#include "base/inputctl.h"
#include "base/inputtranslator.h"

#include "elements/label.h"
#include "elements/text.h"
#include "elements/draw.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>

struct stui2 {
	struct shm_allocator_pdata pd;
	int is_parent;
	shmptr_of(struct window) main_win;
	struct sigaction prev_sigact;
	union {
		struct {
			shmptr_of(struct dblbuf) output_db;
			shmptr_of(struct input_translator) it;
			struct termios term;
		} parent_data;
		struct {
		} child_data;
	};
};

struct common_data {
	shmptr_of(char) parent_compilation_date;
	shmptr_of(char) parent_compilation_time;
};

static struct stui2 *current_stui2;
struct stui2 *global_stui2;

static void global_sigwinch_handler(int sig);
static void resize_elements(struct shm_allocator_pdata *pd, struct window *win);
static void r_resize_elements(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) node);
static int  r_render_z_index(struct shm_allocator_pdata *pd, shmptr_of(struct screen) scr, shmptr_of(struct z_index_node) node);

struct stui2 *
init_stui2()
{
	struct stui2 *stui2;
	struct window *pwin;
	struct dblbuf *pdb;
	struct sigaction sigact;
	struct winsize ws;
	scrcoord width, height;
	struct input_translator *pit;
	struct common_data *pcommon;
	shmptr_of(char) p_comp_date;
	shmptr_of(char) p_comp_time;

	stui2 = malloc(sizeof(struct stui2));
	if(! stui2) {
		return NULL;
	}
	stui2->is_parent = shm_is_parent();
	if(init_shm_allocator(&stui2->pd, NULL, stui2->is_parent, sizeof(struct common_data)) != STUI_OK) {
		return NULL;
	}

	if(stui2->is_parent) {
		stui2->main_win = shm_alloc(&stui2->pd, sizeof(struct window));

		sigemptyset(&sigact.sa_mask);
		sigact.sa_flags = 0;
		sigact.sa_handler = global_sigwinch_handler;
		if(sigaction(SIGWINCH, &sigact, &stui2->prev_sigact) == -1) {
			return NULL;
		}

		if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1) {
			return NULL;
		}
		width = ws.ws_col;
		height = ws.ws_row;

		if(init_window(&stui2->pd, stui2->main_win, width, height) != STUI_OK) {
			return NULL;
		}
		stui2->parent_data.output_db = shm_alloc(&stui2->pd, sizeof(struct dblbuf));
		if(init_dblbuf(&stui2->pd, stui2->parent_data.output_db, width, height) != STUI_OK) {
			return NULL;
		}
		pwin = fromshmptr(struct window, stui2->pd, stui2->main_win);
		pdb = fromshmptr(struct dblbuf, stui2->pd, stui2->parent_data.output_db);
		pwin->ins.target_scr = toshmptr(stui2->pd, &pdb->cur_scr);

		/*                                                                  save cursor position, save screen, hide cursor, clear */
		if(append_io_buffer(&stui2->pd, toshmptr(stui2->pd, &pdb->outbuf), "\e7\e[?1049h\e[?25l\e[2J") != STUI_OK) {
			return NULL;
		}
		pwin = fromshmptr(struct window, stui2->pd, stui2->main_win);
		pdb = fromshmptr(struct dblbuf, stui2->pd, stui2->parent_data.output_db);
		pwin->ins.target_scr = toshmptr(stui2->pd, &pdb->cur_scr);

		stui2->parent_data.it = shm_alloc(&stui2->pd, sizeof(struct input_translator));
		if(stui2->parent_data.it == SHMNULL) {
			return NULL;
		}
		pwin = fromshmptr(struct window, stui2->pd, stui2->main_win);
		pdb = fromshmptr(struct dblbuf, stui2->pd, stui2->parent_data.output_db);
		pit = fromshmptr(struct input_translator, stui2->pd, stui2->parent_data.it);

		init_input_translator(pit);

		tcgetattr(STDIN_FILENO, &stui2->parent_data.term);
		stui2->parent_data.term.c_lflag &= ~ECHO & ~ICANON & ~ISIG & ~IEXTEN & ~ICRNL;
		tcsetattr(STDIN_FILENO, 0, &stui2->parent_data.term);

		p_comp_date = shm_alloc(&stui2->pd, sizeof(__DATE__));
		if(p_comp_date == SHMNULL) {
			return NULL;
		}
		p_comp_time = shm_alloc(&stui2->pd, sizeof(__TIME__));
		if(p_comp_time == SHMNULL) {
			return NULL;
		}
		pcommon = fromshmptr(struct common_data, stui2->pd, shm_first_used(&stui2->pd));
		pcommon->parent_compilation_date =  p_comp_date;
		pcommon->parent_compilation_time =  p_comp_time;
		strcpy(fromshmptr(char, stui2->pd, pcommon->parent_compilation_date), __DATE__);
		strcpy(fromshmptr(char, stui2->pd, pcommon->parent_compilation_time), __TIME__);
	} else {
		/* TODO */
		exit(1);
	}

	current_stui2 = stui2; return stui2;
}

int
exit_stui2(struct stui2 *stui2)
{
	struct window *pwin;
	struct dblbuf *pdb;

	pwin = fromshmptr(struct window, stui2->pd, stui2->main_win);

	sigaction(SIGWINCH, &current_stui2->prev_sigact, NULL);

	current_stui2 = NULL;

	free_window(&stui2->pd, pwin);

	if(stui2->is_parent) {
		pdb = fromshmptr(struct dblbuf, stui2->pd, stui2->parent_data.output_db);
		/*                                                                  restore cursor position, restore screen, show cursor */
		if(append_io_buffer(&stui2->pd, toshmptr(stui2->pd, &pdb->outbuf), "\e8\e[?1049l\e[?25h") != STUI_OK) {
			return STUI_ERR;
		}
		pdb = fromshmptr(struct dblbuf, stui2->pd, stui2->parent_data.output_db);
		if(dump_io_buffer(stui2->pd, &pdb->outbuf, STDOUT_FILENO) != STUI_OK) {
			return STUI_ERR;
		}

		free_dblbuf(&stui2->pd, stui2->parent_data.output_db);
		free_input_translator(&stui2->pd, stui2->parent_data.it);

		stui2->parent_data.term.c_lflag |= ECHO | ICANON | ISIG | IEXTEN | ICRNL;
		tcsetattr(STDIN_FILENO, 0, &stui2->parent_data.term);
	}

	if(free_shm_allocator(stui2->pd, stui2->is_parent) != STUI_OK) {
		return STUI_ERR;
	}


	free(stui2);

	return STUI_OK;
}

int
stui2_is_parent(struct stui2 *stui2)
{
	return stui2->is_parent;
}

stui2_info
stui2_get_info(struct stui2 *stui2)
{
	stui2_info ret;

	ret = 0;

	ret &= ~(INFO_HAS_AGFX | INFO_USES_AGFX);
	ret &= ~(INFO_HAS_AGFX_FB | INFO_USES_AGFX_FB);
	ret &= ~(INFO_HAS_AGFX_DRI | INFO_USES_AGFX_DRI);

	return ret;
}

char const *
stui2_cur_comp_date()
{
	return __DATE__;
}

char const *
stui2_cur_comp_time()
{
	return __TIME__;
}

void
stui2_parent_comp_date(struct stui2 *stui2, char *out, unsigned int len)
{
	struct common_data *pcommon;

	pcommon = fromshmptr(struct common_data, stui2->pd, shm_first_used(&stui2->pd));
	strncpy(out, fromshmptr(char, stui2->pd, pcommon->parent_compilation_date), len);
}

void
stui2_parent_comp_time(struct stui2 *stui2, char *out, unsigned int len)
{
	struct common_data *pcommon;

	pcommon = fromshmptr(struct common_data, stui2->pd, shm_first_used(&stui2->pd));
	strncpy(out, fromshmptr(char, stui2->pd, pcommon->parent_compilation_time), len);
}

void stui2_get_shm_id(struct stui2 *stui2, char *out, unsigned int len)
{
	strncpy(out, stui2->pd.shm.shm_name, len);
}

stui2_window
stui2_main_window(struct stui2 *stui2)
{
	return stui2->main_win;
}

stui2_insertable
stui2_win_get_insertable(struct stui2 *stui2, stui2_window win)
{
	struct window *pwin;

	pwin = fromshmptr(struct window, stui2->pd, win);

	return toshmptr(stui2->pd, &pwin->ins);
}

stui2_element
stui2_create_element(struct stui2 *stui2, stui2_insertable ins, element_z_index z_index, enum stui2_element_type type, ...)
{
	va_list args;
	struct element el;
	shmptr_of(struct element) shmel;
	struct stui2_insertable *pins;

	shm_access(&stui2->pd);
	pins = fromshmptr(struct stui2_insertable, stui2->pd, ins);

	el.z_index = z_index;
	el.scr_pos = (struct scr_rect){0};
	el.pos = (struct rect){0};
	el.render_output = pins->target_scr;
	el.flags = 0;
	el.parent_insertable = ins;

	shmel = z_index_list_insert(&stui2->pd, &pins->root, z_index, el);
	if(shmel == SHMNULL) {
		shm_leave(&stui2->pd);
		return STUI_INVH;
	}
	pins = fromshmptr(struct stui2_insertable, stui2->pd, ins);

	va_start(args, type);
	dispatch_init_element(&stui2->pd, shmel, type, args);
	va_end(args);

	shm_leave(&stui2->pd);

	return shmel;
}

int
stui2_free_element(struct stui2 *stui2, stui2_element el)
{
	struct element *pel;
	struct stui2_insertable *pins;
	shmptr_of(struct z_index_node) node;

	shm_access(&stui2->pd);
	
	dispatch_free_element(&stui2->pd, el);

	pel = fromshmptr(struct element, stui2->pd, el);
	pins = fromshmptr(struct stui2_insertable, stui2->pd, pel->parent_insertable);
	node = pins->root;
	if(z_index_remove(&stui2->pd, &node, el) != STUI_OK) {
		shm_leave(&stui2->pd);
		return STUI_ERR;
	}
	pel = fromshmptr(struct element, stui2->pd, el);
	pins = fromshmptr(struct stui2_insertable, stui2->pd, pel->parent_insertable);
	pins->root = node;

	return STUI_OK;
}

void
stui2_set_position(struct stui2 *stui2, stui2_element el, struct rect rect)
{
	struct element *pel;

	pel = fromshmptr(struct element, stui2->pd, el);
	pel->pos = rect;

	element_resize(stui2->pd, pel);
	dispatch_element_resize(&stui2->pd, el);
}

int
stui2_render(struct stui2 *stui2, stui2_window win)
{
	struct window *pwin;

	pwin = fromshmptr(struct window, stui2->pd, win);

	return r_render_z_index(&stui2->pd, pwin->ins.target_scr, pwin->ins.root);
}

int
stui2_flush(struct stui2 *stui2)
{
	if(stui2->is_parent) {
		return dump_dblbuf(&stui2->pd, stui2->parent_data.output_db, STDOUT_FILENO);
	}
	return STUI_OK;
}

int
stui2_get_input(struct stui2 *stui2, stui2_window win, int timeout)
{
	struct window *pwin;

	if(! stui2->is_parent) {
		return STUI_OK;
	}
	pwin = fromshmptr(struct window, stui2->pd, win);

	input_translator_set_target(stui2->pd, stui2->parent_data.it, toshmptr(stui2->pd, &pwin->input_list));

	if(run_input_translator(&stui2->pd, stui2->parent_data.it, STDIN_FILENO, timeout) != STUI_OK) {
		return STUI_ERR;
	}

	return STUI_OK;
}

struct input_evt
stui2_next_event(struct stui2 *stui2, stui2_window win)
{
	struct window *pwin;

	pwin = fromshmptr(struct window, stui2->pd, win);
	return get_input_ctl(stui2->pd, &pwin->input_list);
}

scrcoord
stui2_insertable_width(struct stui2 *stui2, stui2_insertable ins)
{
	return fromshmptr(struct stui2_insertable, stui2->pd, ins)->width;
}

scrcoord
stui2_insertable_height(struct stui2 *stui2, stui2_insertable ins)
{
	return fromshmptr(struct stui2_insertable, stui2->pd, ins)->height;
}

static void
global_sigwinch_handler(int sig)
{
	struct winsize ws;
	scrcoord width, height;

	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1) {
		return;
	}
	width = ws.ws_col;
	height = ws.ws_row;

	shm_access(&current_stui2->pd);

	if(resize_window(&current_stui2->pd, current_stui2->main_win, width, height) != STUI_OK) {
		shm_leave(&current_stui2->pd);
		return;
	}
	if(resize_dblbuf(&current_stui2->pd, current_stui2->parent_data.output_db, width, height) != STUI_OK) {
		shm_leave(&current_stui2->pd);
		return;
	}

	resize_elements(&current_stui2->pd, fromshmptr(struct window, current_stui2->pd, current_stui2->main_win));

	shm_leave(&current_stui2->pd);
}

static void
resize_elements(struct shm_allocator_pdata *pd, struct window *win)
{
	r_resize_elements(pd, win->ins.root);
}

static void
r_resize_elements(struct shm_allocator_pdata *pd, shmptr_of(struct z_index_node) node)
{
	struct z_index_node *pnode;
	shmptr_of(struct element_list_node) list;
	struct element_list_node *plist;
	struct element *pelement;

	if(node == SHMNULL) {
		return;
	}

	pnode = fromshmptr(struct z_index_node, *pd, node);
	r_resize_elements(pd, pnode->left);
	pnode = fromshmptr(struct z_index_node, *pd, node);
	r_resize_elements(pd, pnode->right);
	pnode = fromshmptr(struct z_index_node, *pd, node);

	list = pnode->list.head;
	while(list != SHMNULL) {
		plist = fromshmptr(struct element_list_node, *pd, list);
		pelement = &plist->el;
		element_resize(*pd, pelement);
		dispatch_element_resize(pd, toshmptr(*pd, pelement));
		plist = fromshmptr(struct element_list_node, *pd, list);

		list = plist->next;
	}
}

static int
r_render_z_index(struct shm_allocator_pdata *pd, shmptr_of(struct screen) scr, shmptr_of(struct z_index_node) node)
{
	struct z_index_node *pnode;
	struct element_list_node *plist;
	struct element *pelement;

	if(node == SHMNULL) {
		return STUI_OK;
	}

	shm_access(pd);

	pnode = fromshmptr(struct z_index_node, *pd, node);
	if(r_render_z_index(pd, scr, pnode->left) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}

	plist = fromshmptr(struct element_list_node, *pd, pnode->list.head);
	while(plist != SHMNULL) {
		pelement = &plist->el;
		if(dispatch_element_draw(pd, toshmptr(*pd, pelement)) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}

		plist = fromshmptr(struct element_list_node, *pd, plist->next);
	}

	if(r_render_z_index(pd, scr, pnode->right) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}

	shm_leave(pd);
	return STUI_OK;
}

int
stui2_label_set_style(struct stui2 *stui2, stui2_element label, struct char_cell cc)
{
	struct element *plabel;

	plabel = fromshmptr(struct element, stui2->pd, label);
	if(plabel->data.type != ELEMENT_LABEL) {
		return STUI_ERR;
	}

	return label_set_style(stui2->pd, plabel, cc);
}

int
stui2_label_set_string(struct stui2 *stui2, stui2_element label, char *string)
{
	struct element *plabel;

	plabel = fromshmptr(struct element, stui2->pd, label);
	if(plabel->data.type != ELEMENT_LABEL) {
		return STUI_ERR;
	}

	return label_set_string(&stui2->pd, label, string);
}

int
stui2_text_append(struct stui2 *stui2, stui2_element text, struct char_cell cc, char *str)
{
	struct element *ptext;

	ptext = fromshmptr(struct element, stui2->pd, text);
	if(ptext->data.type != ELEMENT_TEXT) {
		return STUI_ERR;
	}
	return text_append(&stui2->pd, text, cc, str);
}

int
stui2_text_printf(struct stui2 *stui2, stui2_element text, struct char_cell cc, char *format, ...)
{
	va_list args, copy;
	char *buff;
	unsigned int len;

	va_start(args, format);

	va_copy(copy, args);
	len = vsnprintf(NULL, 0, format, copy);
	va_end(copy);

	buff = malloc(len + 1);
	if(! buff) {
		return STUI_ERR;
	}

	va_copy(copy, args);
	vsprintf(buff, format, copy);
	va_end(copy);
	buff[len] = '\0';

	if(stui2_text_append(stui2, text, cc, buff) != STUI_OK) {
		free(buff);
		return STUI_ERR;
	}

	free(buff);
	return STUI_OK;
}

int
stui2_draw_set(struct stui2 * stui2, stui2_element el, scrcoord x, scrcoord y, struct char_cell cc)
{
	struct element *pdraw;

	pdraw = fromshmptr(struct element, stui2->pd, el);
	if(pdraw->data.type != ELEMENT_DRAW) {
		return STUI_ERR;
	}
	return draw_setcc(stui2->pd, el, x, y, cc);
}

int
stui2_draw_line(struct stui2 *stui2, stui2_element el, scrcoord x0, scrcoord y0, scrcoord x1, scrcoord y1, struct char_cell cc)
{
	struct element *pdraw;

	pdraw = fromshmptr(struct element, stui2->pd, el);
	if(pdraw->data.type != ELEMENT_DRAW) {
		return STUI_ERR;
	}
	return draw_mkline(stui2->pd, el, x0, y0, x1, y1, cc);
}

int
stui2_draw_rect(struct stui2 *stui2, stui2_element el, scrcoord x, scrcoord y, scrcoord width, scrcoord height, struct char_cell fill)
{
	struct element *pdraw;

	pdraw = fromshmptr(struct element, stui2->pd, el);
	if(pdraw->data.type != ELEMENT_DRAW) {
		return STUI_ERR;
	}
	return draw_rect(stui2->pd, el, x, y, width, height, fill);
}

