#ifndef DBLBUF_H
#define DBLBUF_H

#include "shm/allocator.h"
#include "base/iobuffer.h"
#include "base/screen.h"

struct dblbuf {
	struct screen cur_scr;
	struct screen prev_scr;
	struct io_buffer outbuf;

	int is_in_color_escape;
	struct color last_fg;
	struct color last_bg;
	scrcoord last_diff_line;
	scrcoord last_diff_idx;
	int is_redrawing;
};

int  init_dblbuf  (struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, scrcoord w, scrcoord h);
void free_dblbuf  (struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db);
int  resize_dblbuf(struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, scrcoord new_width, scrcoord new_height);
int  set_cell     (struct shm_allocator_pdata  pd, struct dblbuf *db, struct char_cell cc, scrcoord x, scrcoord y);
int  dump_dblbuf  (struct shm_allocator_pdata *pd, shmptr_of(struct dblbuf) db, int fd);

#endif

