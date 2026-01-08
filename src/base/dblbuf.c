#include <inttypes.h>
#include <stddef.h>

#include "base/dblbuf.h"
#include "base/iobuffer.h"

#define ANSI_ESC		"\033["
#define ANSI_GOTO(X,Y)	ANSI_ESC Y ";" X "H" /* 1-indexed */
#define GET_CC(PD,SCR,X,Y) (fromshmptr(struct char_cell,PD,cell_at_screen(PD,SCR,X,Y)))

static int  begin_color_escape(struct dblbuf *db);
static int  end_color_escape(struct dblbuf *db);
static int  move_to(struct io_buffer *buf, scrcoord x, scrcoord y);
static int  output_char(struct io_buffer *buf, struct char_cell cc);
static int  output_fg(struct dblbuf *db, struct char_cell cc);
static int  output_bg(struct dblbuf *db, struct char_cell cc);
static int  output_cc(struct dblbuf *db, struct char_cell cc);
static int  is_different(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord x, scrcoord y);
static scrcoord difference_idx(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord line);
static int  redraw_cc(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord x, scrcoord y);
static int  redraw_line(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord line);
static int  redraw_dblbuf(struct shm_allocator_pdata pd, struct dblbuf *db);

int
init_dblbuf(struct shm_allocator_pdata *pd, struct dblbuf *db, scrcoord w, scrcoord h)
{
	db->is_in_color_escape = 0;
	db->last_diff_line = db->last_diff_idx = -1;
	db->is_redrawing = 0;
	init_io_buffer(&db->outbuf);
	if(init_screen(pd, &db->cur_scr, w, h) != STUI_OK) return STUI_ERR;
	if(init_screen(pd, &db->prev_scr, w, h) != STUI_OK) return STUI_ERR;

	return STUI_OK;
}

void
free_dblbuf(struct shm_allocator_pdata *pd, struct dblbuf *db)
{
	free_io_buffer(&db->outbuf);

	free_screen(pd, db->cur_scr);
	free_screen(pd, db->prev_scr);
}

int
resize_dblbuf(struct shm_allocator_pdata *pd, struct dblbuf *db, scrcoord new_width, scrcoord new_height)
{
	free_dblbuf(pd, db);
	return init_dblbuf(pd, db, new_width, new_height);
}

int
set_cell(struct shm_allocator_pdata pd, struct dblbuf *db, struct char_cell cc, scrcoord x, scrcoord y)
{
	return set_cell_screen(pd, db->cur_scr, cc, x, y);
}

int
dump_dblbuf(struct shm_allocator_pdata *pd, struct dblbuf *db, int fd)
{
	if(redraw_dblbuf(*pd, db) != STUI_OK) return STUI_ERR;
	if(copy_screen(pd, &db->prev_scr, db->cur_scr) != STUI_OK) return STUI_ERR;
	return dump_io_buffer(&db->outbuf, fd);
}

static int
begin_color_escape(struct dblbuf *db)
{
	if(! db->is_in_color_escape) {
		db->is_in_color_escape = 1;
		return append_io_buffer(&db->outbuf, ANSI_ESC);
	} else {
		return append_io_buffer(&db->outbuf, ";");
	}
}

static int
end_color_escape(struct dblbuf *db)
{
	if(db->is_in_color_escape) {
		db->is_in_color_escape = 0;
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
is_different(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord x, scrcoord y)
{
	return !color_eq(GET_CC(pd,db->cur_scr,x,y)->fg, GET_CC(pd,db->prev_scr,x,y)->fg) ||
	       !color_eq(GET_CC(pd,db->cur_scr,x,y)->bg, GET_CC(pd,db->prev_scr,x,y)->bg) || 
	       GET_CC(pd,db->cur_scr,x,y)->c != GET_CC(pd,db->prev_scr,x,y)->c;
}

static scrcoord
difference_idx(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord line)
{
	scrcoord i;

	if(db->last_diff_line == line) {
		i = db->last_diff_idx + 1;
	} else {
		i = 0;
	}

	for(; i < db->cur_scr.width; ++i) {
		if(is_different(pd, db, i, line)) {
			db->last_diff_idx = i;
			db->last_diff_line = line;
			return i;
		}
	}

	return -1;
}

static int
redraw_cc(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord x, scrcoord y)
{
	if(db->is_redrawing) {
		if(! color_eq(GET_CC(pd,db->cur_scr,x,y)->fg, db->last_fg)) {
			db->last_fg = GET_CC(pd,db->cur_scr,x,y)->fg;
			if(output_fg(db, *GET_CC(pd,db->cur_scr,x,y)) != STUI_OK) return STUI_ERR;
		}
		if(! color_eq(GET_CC(pd,db->cur_scr,x,y)->bg, db->last_bg)) {
			db->last_bg = GET_CC(pd,db->cur_scr,x,y)->bg;
			if(output_bg(db, *GET_CC(pd,db->cur_scr,x,y)) != STUI_OK) return STUI_ERR;
		}
		if(end_color_escape(db) != STUI_OK) return STUI_ERR;
		return output_char(&db->outbuf, *GET_CC(pd,db->cur_scr,x,y));
	} else {
		db->is_redrawing = 1;
		if(move_to(&db->outbuf, x, y) != STUI_OK) return STUI_ERR;
		return output_cc(db, *GET_CC(pd,db->cur_scr,x,y));
	}
}

static int
redraw_line(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord line)
{
	scrcoord i;

	while((i = difference_idx(pd, db, line)) != (scrcoord)-1) {
		if(redraw_cc(pd, db, i, line) != STUI_OK) return STUI_ERR;
	}

	return STUI_OK;
}

static int
redraw_dblbuf(struct shm_allocator_pdata pd, struct dblbuf *db)
{
	scrcoord ln;

	db->is_redrawing = 0;
	db->last_diff_line = db->last_diff_idx = -1;

	for(ln = 0; ln < db->cur_scr.height; ++ln) {
		if(redraw_line(pd, db, ln) != STUI_OK) return STUI_ERR;
	}
	
	return STUI_OK;
}

