#include <inttypes.h>
#include <stddef.h>

#include "base/dblbuf.h"
#include "base/iobuffer.h"

#define ANSI_ESC		"\033["
#define ANSI_GOTO(X,Y)	ANSI_ESC Y ";" X "H" /* 1-indexed */
#define GET_CC(PD,SCR,X,Y) (fromshmptr(struct char_cell,PD,cell_at_screen(PD,SCR,X,Y)))

static int  begin_color_escape(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db);
static int  end_color_escape(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db);
static int  move_to(struct shm_allocator_pdata *pd, shmptr_of(struct io_buffer) buf, scrcoord x, scrcoord y);
static int  output_char(struct shm_allocator_pdata *pd, shmptr_of(struct io_buffer) buf, struct char_cell cc);
static int  output_fg(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, struct char_cell cc);
static int  output_bg(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, struct char_cell cc);
static int  output_cc(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, struct char_cell cc);
static int  is_different(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord x, scrcoord y);
static scrcoord difference_idx(struct shm_allocator_pdata pd, struct dblbuf *db, scrcoord line);
static int  redraw_cc(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, scrcoord x, scrcoord y);
static int  redraw_line(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, scrcoord line);
static int  redraw_dblbuf(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db);

int
init_dblbuf(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, scrcoord w, scrcoord h)
{
	struct dblbuf *pdb;

	shm_access(pd);

	pdb = fromshmptr(struct dblbuf, *pd, db);
	pdb->is_in_color_escape = 0;
	pdb->last_diff_line = pdb->last_diff_idx = -1;
	pdb->is_redrawing = 0;
	init_io_buffer(&pdb->outbuf);
	if(init_screen(pd, toshmptr(*pd, &pdb->cur_scr), w, h) != STUI_OK) return STUI_ERR;
	pdb = fromshmptr(struct dblbuf, *pd, db);
	if(init_screen(pd, toshmptr(*pd, &pdb->prev_scr), w, h) != STUI_OK) return STUI_ERR;

	return STUI_OK;
}

void
free_dblbuf(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db)
{
	struct dblbuf *pdb;

	shm_access(pd);

	pdb = fromshmptr(struct dblbuf, *pd, db);
	free_io_buffer(pd, pdb->outbuf);

	free_screen(pd, pdb->cur_scr);
	free_screen(pd, pdb->prev_scr);

	shm_leave(pd);
}

int
resize_dblbuf(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, scrcoord new_width, scrcoord new_height)
{
	free_dblbuf(pd, db);
	return init_dblbuf(pd, db, new_width, new_height);
}

int
set_cell(struct shm_allocator_pdata pd, struct dblbuf *db, struct char_cell cc, scrcoord x, scrcoord y)
{
	if(set_cell_screen(pd, db->cur_scr, cc, x, y) != STUI_OK) {
		return STUI_ERR;
	}

	return STUI_OK;
}

int
dump_dblbuf(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, int fd)
{
	struct dblbuf *pdb;

	shm_access(pd);

	if(redraw_dblbuf(pd, db) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}
	pdb = fromshmptr(struct dblbuf, *pd, db);
	if(copy_screen(pd, toshmptr(*pd, &pdb->prev_scr), pdb->cur_scr) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}
	pdb = fromshmptr(struct dblbuf, *pd, db);
	if(dump_io_buffer(*pd, &pdb->outbuf, fd) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}

	shm_leave(pd);
	return STUI_OK;
}

static int
begin_color_escape(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db)
{
	struct dblbuf *pdb;

	shm_access(pd);

	pdb = fromshmptr(struct dblbuf, *pd, db);
	if(! pdb->is_in_color_escape) {
		pdb->is_in_color_escape = 1;
		if(append_io_buffer(pd, toshmptr(*pd, &pdb->outbuf), ANSI_ESC) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
	} else {
		if(append_io_buffer(pd, toshmptr(*pd, &pdb->outbuf), ";") != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
	}
	
	shm_leave(pd);
	return STUI_OK;
}

static int
end_color_escape(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db)
{
	struct dblbuf *pdb;

	shm_access(pd);

	pdb = fromshmptr(struct dblbuf, *pd, db);
	if(pdb->is_in_color_escape) {
		pdb->is_in_color_escape = 0;
		if(append_io_buffer(pd, toshmptr(*pd, &pdb->outbuf), "m") != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
	}

	shm_leave(pd);
	return STUI_OK;
}

static int
move_to(struct shm_allocator_pdata *pd, shmptr_of(struct io_buffer) buf, scrcoord x, scrcoord y)
{
	shm_access(pd);

	if(printf_io_buffer(pd, buf, ANSI_GOTO("%u","%u"), (unsigned int)y+1, (unsigned int)x+1) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}

	shm_leave(pd);
	return STUI_OK;
}

static int
output_char(struct shm_allocator_pdata *pd, shmptr_of(struct io_buffer) buf, struct char_cell cc)
{
	return write_io_buffer(pd, buf, &cc.c, 1);
}

static int
output_fg(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, struct char_cell cc)
{
	struct dblbuf *pdb;

	shm_access(pd);
	begin_color_escape(pd, db);

	pdb = fromshmptr(struct dblbuf, *pd, db);
	switch(cc.fg.type) {
	case  CLR_USR:
		if(cc.fg.data.usr < 7) {
			if(printf_io_buffer(pd, toshmptr(*pd, &pdb->outbuf), "3%" PRIu8, cc.fg.data.usr) != STUI_OK) {
				shm_leave(pd);
				return STUI_ERR;
			}
		} else if (cc.fg.data.usr >= 8 && cc.fg.data.usr < 16) {
			if(printf_io_buffer(pd, toshmptr(*pd, &pdb->outbuf), "9%" PRIu8, cc.fg.data.usr - 8) != STUI_OK) {
				shm_leave(pd);
				return STUI_ERR;
			}
		}

		shm_leave(pd);
		return STUI_ERR;
	case  CLR_256:
		if(printf_io_buffer(pd, toshmptr(*pd, &pdb->outbuf), "38;5;%" PRIu8, cc.fg.data.c256) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
		break;
	case  CLR_RGB:
		if(printf_io_buffer(pd, toshmptr(*pd, &pdb->outbuf),
		              "38;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8,
		                      cc.fg.data.rgb.r, cc.fg.data.rgb.g, cc.fg.data.rgb.b) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
		break;
	case  CLR_DEFAULT:
		if(append_io_buffer(pd, toshmptr(*pd, &pdb->outbuf), "39") != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
		break;
	default:
		shm_leave(pd);
		return STUI_ERR;
	}

	shm_leave(pd);
	return STUI_OK;
}

static int
output_bg(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, struct char_cell cc)
{
	struct dblbuf *pdb;

	shm_access(pd);
	begin_color_escape(pd, db);

	pdb = fromshmptr(struct dblbuf, *pd, db);
	switch(cc.fg.type) {
	case  CLR_USR:
		if(cc.fg.data.usr < 7) {
			if(printf_io_buffer(pd, toshmptr(*pd, &pdb->outbuf), "4%" PRIu8, cc.fg.data.usr) != STUI_OK) {
				shm_leave(pd);
				return STUI_ERR;
			}
		} else if (cc.fg.data.usr >= 8 && cc.fg.data.usr < 16) {
			if(printf_io_buffer(pd, toshmptr(*pd, &pdb->outbuf), "10%" PRIu8, cc.fg.data.usr - 8) != STUI_OK) {
				shm_leave(pd);
				return STUI_ERR;
			}
		}

		shm_leave(pd);
		return STUI_ERR;
	case  CLR_256:
		if(printf_io_buffer(pd, toshmptr(*pd, &pdb->outbuf), "48;5;%" PRIu8, cc.fg.data.c256) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
		break;
	case  CLR_RGB:
		if(printf_io_buffer(pd, toshmptr(*pd, &pdb->outbuf),
		              "48;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8,
		                      cc.fg.data.rgb.r, cc.fg.data.rgb.g, cc.fg.data.rgb.b) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
		break;
	case  CLR_DEFAULT:
		if(append_io_buffer(pd, toshmptr(*pd, &pdb->outbuf), "49") != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
		break;
	default:
		shm_leave(pd);
		return STUI_ERR;
	}

	shm_leave(pd);
	return STUI_OK;
}

static int
output_cc(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, struct char_cell cc)
{
	struct dblbuf *pdb;

	shm_access(pd);

	if(output_fg(pd, db, cc) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}
	if(output_bg(pd, db, cc) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}
	if(end_color_escape(pd, db) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}
	pdb = fromshmptr(struct dblbuf, *pd, db);
	if(output_char(pd, toshmptr(*pd, &pdb->outbuf), cc) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}

	shm_leave(pd);
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
redraw_cc(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, scrcoord x, scrcoord y)
{
	struct dblbuf *pdb;

	shm_access(pd);

	pdb = fromshmptr(struct dblbuf, *pd, db);
	if(move_to(pd, toshmptr(*pd, &pdb->outbuf), x, y) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}
	pdb = fromshmptr(struct dblbuf, *pd, db);
	if(pdb->is_redrawing) {
		if(! color_eq(GET_CC(*pd,pdb->cur_scr,x,y)->fg, pdb->last_fg)) {
			pdb->last_fg = GET_CC(*pd,pdb->cur_scr,x,y)->fg;
			if(output_fg(pd, db, *GET_CC(*pd,pdb->cur_scr,x,y)) != STUI_OK) {
				shm_leave(pd);
				return STUI_ERR;
			}
			pdb = fromshmptr(struct dblbuf, *pd, db);
		}
		if(! color_eq(GET_CC(*pd,pdb->cur_scr,x,y)->bg, pdb->last_bg)) {
			pdb->last_bg = GET_CC(*pd,pdb->cur_scr,x,y)->bg;
			if(output_bg(pd, db, *GET_CC(*pd,pdb->cur_scr,x,y)) != STUI_OK) {
				shm_leave(pd);
				return STUI_ERR;
			}
			pdb = fromshmptr(struct dblbuf, *pd, db);
		}
		if(end_color_escape(pd, db) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
		pdb = fromshmptr(struct dblbuf, *pd, db);
		if(output_char(pd, toshmptr(*pd, &pdb->outbuf), *GET_CC(*pd,pdb->cur_scr,x,y)) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
	} else {
		pdb->is_redrawing = 1;
		if(output_cc(pd, db, *GET_CC(*pd,pdb->cur_scr,x,y)) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
	}

	shm_leave(pd);
	return STUI_OK;
}

static int
redraw_line(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, scrcoord line)
{
	struct dblbuf *pdb;
	scrcoord i;

	shm_access(pd);

	pdb = fromshmptr(struct dblbuf, *pd, db);
	while((i = difference_idx(*pd, pdb, line)) != (scrcoord)-1) {
		if(redraw_cc(pd, db, i, line) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
		pdb = fromshmptr(struct dblbuf, *pd, db);
	}

	shm_leave(pd);
	return STUI_OK;
}

static int
redraw_dblbuf(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db)
{
	struct dblbuf *pdb;
	scrcoord ln;

	shm_access(pd);

	pdb = fromshmptr(struct dblbuf, *pd, db);

	pdb->is_redrawing = 0;
	pdb->last_diff_line = pdb->last_diff_idx = -1;

	for(ln = 0; ln < pdb->cur_scr.height; ++ln) {
		if(redraw_line(pd, db, ln) != STUI_OK) {
			shm_leave(pd);
			return STUI_ERR;
		}
		pdb = fromshmptr(struct dblbuf, *pd, db);
	}
	
	shm_leave(pd);
	return STUI_OK;
}

