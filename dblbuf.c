#include <inttypes.h>
#include <stddef.h>

#include "dblbuf.h"

#define ANSI_ESC		"\033["
#define ANSI_GOTO(X,Y)	ANSI_ESC Y ";" X "H" /* 1-indexed */

static int  begin_color_escape(struct dblbuf *db);
static int  end_color_escape(struct dblbuf *db);
static int  move_to(struct io_buffer *buf, scrcoord x, scrcoord y);
static int  output_char(struct io_buffer *buf, struct char_cell cc);
static int  output_fg(struct dblbuf *db, struct char_cell cc);
static int  output_bg(struct dblbuf *db, struct char_cell cc);
static int  output_cc(struct dblbuf *db, struct char_cell cc);
static int  is_different(struct dblbuf *db, scrcoord x, scrcoord y);
static scrcoord difference_idx(struct dblbuf *db, scrcoord line);
static int  redraw_cc(struct dblbuf *db, scrcoord x, scrcoord y);
static int  redraw_line(struct dblbuf *db, scrcoord line);
static int  redraw_dblbuf(struct dblbuf *db);

int
init_dblbuf(struct dblbuf *db, scrcoord w, scrcoord h)
{
	db->width = w;
	db->height = h;
	db->internal.is_in_color_escape = 0;
	db->internal.last_diff_line = db->internal.last_diff_idx = -1;
	db->internal.is_redrawing = 0;
	init_io_buffer(& db->outbuf);
	db->cur_scr = db->prev_scr = NULL;
	if(init_screen(& db->cur_scr, w, h) != STUI_OK) return STUI_ERR;
	if(init_screen(& db->prev_scr, w, h) != STUI_OK) return STUI_ERR;

	return STUI_OK;
}

void
free_dblbuf(struct dblbuf *db)
{
	free_io_buffer(& db->outbuf);

	if(db->cur_scr) free_screen(db->cur_scr, db->width, db->height);
	if(db->prev_scr) free_screen(db->prev_scr, db->width, db->height);
}

int
resize_dblbuf(struct dblbuf *db, scrcoord new_width, scrcoord new_height)
{
	free_dblbuf(db);
	return init_dblbuf(db, new_width, new_height);
}

int
set_cell(struct dblbuf *db, struct char_cell cc, scrcoord x, scrcoord y)
{
	return set_cell_screen(db->cur_scr, db->width, db->height, cc, x, y);
}

int
dump_dblbuf(struct dblbuf *db, int fd)
{
	if(redraw_dblbuf(db) != STUI_OK) return STUI_ERR;
	if(copy_screen(&db->prev_scr, db->cur_scr, db->width, db->height, db->width, db->height) != STUI_OK) return STUI_ERR;
	return dump_io_buffer(&db->outbuf, fd);
}

static int
begin_color_escape(struct dblbuf *db)
{
	if(! db->internal.is_in_color_escape) {
		db->internal.is_in_color_escape = 1;
		return append_io_buffer(&db->outbuf, ANSI_ESC);
	} else {
		return append_io_buffer(&db->outbuf, ";");
	}
}

static int
end_color_escape(struct dblbuf *db)
{
	if(db->internal.is_in_color_escape) {
		db->internal.is_in_color_escape = 0;
		return append_io_buffer(&db->outbuf, "m");
	}

	return STUI_OK;
}

static int
move_to(struct io_buffer *buf, scrcoord x, scrcoord y)
{
	return printf_io_buffer(buf, ANSI_GOTO("%u","%u"), (unsigned int)y+1, (unsigned int)x+1);
}

static int
output_char(struct io_buffer *buf, struct char_cell cc)
{
	return write_io_buffer(buf, &cc.c, 1);
}

static int
output_fg(struct dblbuf *db, struct char_cell cc)
{
	begin_color_escape(db);

	switch(cc.fg.type) {
	case  CLR_USR:
		if(cc.fg.data.usr < 7) {
			return printf_io_buffer(&db->outbuf, "3%" PRIu8, cc.fg.data.usr);
		} else if (cc.fg.data.usr >= 8 && cc.fg.data.usr < 16) {
			return printf_io_buffer(&db->outbuf, "9%" PRIu8, cc.fg.data.usr - 8);
		}

		return STUI_ERR;
	case  CLR_256:
		return printf_io_buffer(&db->outbuf, "38;5;%" PRIu8, cc.fg.data.c256);
	case  CLR_RGB:
		return printf_io_buffer(&db->outbuf,
		              "38;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8,
		                      cc.fg.data.rgb.r, cc.fg.data.rgb.g, cc.fg.data.rgb.b);
	case  CLR_DEFAULT:
		return append_io_buffer(&db->outbuf, "39");
	case  CLR_UNSET:
		return STUI_OK;
	default:
		return STUI_ERR;
	}
}

static int
output_bg(struct dblbuf *db, struct char_cell cc)
{
	begin_color_escape(db);

	switch(cc.fg.type) {
	case  CLR_USR:
		if(cc.fg.data.usr < 7) {
			return printf_io_buffer(&db->outbuf, "4%" PRIu8, cc.fg.data.usr);
		} else if (cc.fg.data.usr >= 8 && cc.fg.data.usr < 16) {
			return printf_io_buffer(&db->outbuf, "10%" PRIu8, cc.fg.data.usr - 8);
		}

		return STUI_ERR;
	case  CLR_256:
		return printf_io_buffer(&db->outbuf, "48;5;%" PRIu8, cc.fg.data.c256);
	case  CLR_RGB:
		return printf_io_buffer(&db->outbuf,
		              "48;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8,
		                      cc.fg.data.rgb.r, cc.fg.data.rgb.g, cc.fg.data.rgb.b);
	case  CLR_DEFAULT:
		return append_io_buffer(&db->outbuf, "49");
	case  CLR_UNSET:
		return STUI_OK;
	default:
		return STUI_ERR;
	}
}

static int
output_cc(struct dblbuf *db, struct char_cell cc)
{
	if(output_fg(db, cc) != STUI_OK) return STUI_ERR;
	if(output_bg(db, cc) != STUI_OK) return STUI_ERR;
	if(end_color_escape(db) != STUI_OK) return STUI_ERR;
	if(output_char(&db->outbuf, cc) != STUI_OK) return STUI_ERR;
	return STUI_OK;
}

static int
is_different(struct dblbuf *db, scrcoord x, scrcoord y)
{
	return !color_eq(db->cur_scr[x][y].fg, db->prev_scr[x][y].fg) ||
	       !color_eq(db->cur_scr[x][y].bg, db->prev_scr[x][y].bg) || 
	       db->cur_scr[x][y].c != db->prev_scr[x][y].c;
}

static scrcoord
difference_idx(struct dblbuf *db, scrcoord line)
{
	scrcoord i;

	if(db->internal.last_diff_line == line) {
		i = db->internal.last_diff_idx + 1;
	} else {
		i = 0;
	}

	for(; i < db->width; ++i) {
		if(is_different(db, i, line)) {
			db->internal.last_diff_idx = i;
			db->internal.last_diff_line = line;
			return i;
		}
	}

	return -1;
}

static int
redraw_cc(struct dblbuf *db, scrcoord x, scrcoord y)
{
	if(db->internal.is_redrawing) {
		if(! color_eq(db->cur_scr[x][y].fg, db->internal.last_fg)) {
			db->internal.last_fg = db->cur_scr[x][y].fg;
			if(output_fg(db, db->cur_scr[x][y]) != STUI_OK) return STUI_ERR;
		}
		if(! color_eq(db->cur_scr[x][y].bg, db->internal.last_bg)) {
			db->internal.last_bg = db->cur_scr[x][y].bg;
			if(output_bg(db, db->cur_scr[x][y]) != STUI_OK) return STUI_ERR;
		}
		if(end_color_escape(db) != STUI_OK) return STUI_ERR;
		return output_char(&db->outbuf, db->cur_scr[x][y]);
	} else {
		db->internal.is_redrawing = 1;
		if(move_to(&db->outbuf, x, y) != STUI_OK) return STUI_ERR;
		return output_cc(db, db->cur_scr[x][y]);
	}
}

static int
redraw_line(struct dblbuf *db, scrcoord line)
{
	scrcoord i;

	while((i = difference_idx(db, line)) != (scrcoord)-1) {
		if(redraw_cc(db, i, line) != STUI_OK) return STUI_ERR;
	}

	return STUI_OK;
}

static int
redraw_dblbuf(struct dblbuf *db)
{
	scrcoord ln;

	db->internal.is_redrawing = 0;
	db->internal.last_diff_line = db->internal.last_diff_idx = -1;

	for(ln = 0; ln < db->height; ++ln) {
		if(redraw_line(db, ln) != STUI_OK) return STUI_ERR;
	}
	
	return STUI_OK;
}

