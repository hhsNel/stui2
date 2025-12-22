#ifndef DBLBUF_H
#define DBLBUF_H

#include "iobuffer.h"
#include "screen.h"

struct dblbuf {
	struct io_buffer outbuf;
	struct char_cell **cur_scr;
	struct char_cell **prev_scr;
	scrcoord width, height;

	struct {
		int is_in_color_escape;
		struct color last_fg;
		struct color last_bg;
		scrcoord last_diff_line;
		scrcoord last_diff_idx;
		int is_redrawing;
	} internal;
};

int  init_dblbuf  (struct dblbuf *db, scrcoord w, scrcoord h);
void free_dblbuf  (struct dblbuf *db);
int  resize_dblbuf(struct dblbuf *db, scrcoord new_width, scrcoord new_height);
int  set_cell     (struct dblbuf *db, struct char_cell cc, scrcoord x, scrcoord y);
int  dump_dblbuf  (struct dblbuf *db, int fd);

#endif

