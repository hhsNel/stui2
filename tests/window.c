#include "layout/window.h"
#include "elements/dispatch.h"
#include "base/dblbuf.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void mklabel(struct shm_allocator_pdata *pd, shmptr_of(struct element) label, ...) {
	va_list args;

	va_start(args, label);
	dispatch_init_element(pd, label, ELEMENT_LABEL, args);
	va_end(args);
}

int main(int argc, char **argv) {
	struct shm_allocator_pdata pd;
	shmptr_of(struct window) w;
	struct window *pw;
	shmptr_of(struct element) label;
	struct element *plabel;
	struct char_cell cc;
	shmptr_of(struct dblbuf) db;
	struct dblbuf *pdb;

	init_shm_allocator(&pd, NULL, 1, 0);

	w = shm_alloc(&pd, sizeof(struct window));
	pw = fromshmptr(struct window, pd, w);

	init_window(&pd, w, 32, 32);
	pw = fromshmptr(struct window, pd, w);

	label = shm_alloc(&pd, sizeof(struct element));
	pw = fromshmptr(struct window, pd, w);
	plabel = fromshmptr(struct element, pd, label);
	plabel->z_index = 0;
	plabel->scr_pos.x = 4;
	plabel->scr_pos.y = 6;
	plabel->scr_pos.width = 4;
	plabel->scr_pos.height = 8;
	plabel->render_output = toshmptr(pd, &pw->scr);
	z_index_list_insert(&pd, &pw->elements, 0, label);
	pw = fromshmptr(struct window, pd, w);
	plabel = fromshmptr(struct element, pd, label);

	cc.attr = 0;
	cc.fg.type = cc.bg.type = CLR_DEFAULT;
	mklabel(&pd, label, cc, "this is some text that is too long for this label");
	pw = fromshmptr(struct window, pd, w);
	plabel = fromshmptr(struct element, pd, label);

	dispatch_element_draw(&pd, label);
	db = shm_alloc(&pd, sizeof(struct dblbuf));
	pw = fromshmptr(struct window, pd, w);

	init_dblbuf(&pd, db, 32, 32);
	pw = fromshmptr(struct window, pd, w);
	pdb = fromshmptr(struct dblbuf, pd, db);

	free_screen(&pd, pdb->cur_scr);
	pw = fromshmptr(struct window, pd, w);
	pdb = fromshmptr(struct dblbuf, pd, db);
	pdb->cur_scr = pw->scr;
 	dump_dblbuf(&pd, db, STDOUT_FILENO);

	free_shm_allocator(pd, 1);
}

