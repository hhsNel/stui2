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
	shmptr_of(struct element) shmlabel;
	struct element label;
	struct char_cell cc;
	shmptr_of(struct dblbuf) db;
	struct dblbuf *pdb;

	init_shm_allocator(&pd, NULL, 1, 0);

	w = shm_alloc(&pd, sizeof(struct window));
	pw = fromshmptr(struct window, pd, w);

	init_window(&pd, w, 32, 32);
	pw = fromshmptr(struct window, pd, w);

	label.z_index = 0;
	label.scr_pos.x = 4;
	label.scr_pos.y = 6;
	label.scr_pos.width = 4;
	label.scr_pos.height = 8;
	label.render_output = toshmptr(pd, &pw->scr);
	shmlabel = z_index_list_insert(&pd, &pw->elements, 0, label);
	pw = fromshmptr(struct window, pd, w);

	cc.attr = 0;
	cc.fg.type = cc.bg.type = CLR_DEFAULT;
	mklabel(&pd, shmlabel, cc, "this is some text that is too long for this label");
	pw = fromshmptr(struct window, pd, w);

	dispatch_element_draw(&pd, shmlabel);
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

