#include "shm/allocator.h"
#include "base/inputtranslator.h"
#include "base/iobuffer.h"
#include "base/screen.h"
#include "base/dblbuf.h"
#include "base/inputctl.h"

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

int main(int argc, char **argv) {
	struct shm_allocator_pdata pd;
 	shmptr_of(struct io_buffer) buf;
 	struct io_buffer *pbuf;
 	shmptr_of(struct screen) scr;
 	struct screen *pscr;
 	struct char_cell cc;
 	shmptr_of(struct dblbuf) db;
 	struct dblbuf *pdb;
 	shmptr_of(struct input_ctl) ic;
 	struct input_ctl *pic;
 	struct input_evt evt;
 	shmptr_of(struct input_translator) it;
 	struct input_translator *pit;

	init_shm_allocator(&pd, NULL, 1, 0);

	buf = shm_alloc(&pd, sizeof(struct io_buffer));
	pbuf = fromshmptr(struct io_buffer, pd, buf);
 
 	init_io_buffer(pbuf);
 	append_io_buffer(&pd, buf, "Hello, World!\n");
	pbuf = fromshmptr(struct io_buffer, pd, buf);
 	dump_io_buffer(pd, pbuf, STDOUT_FILENO);

	scr = shm_alloc(&pd, sizeof(struct screen));
	pscr = fromshmptr(struct screen, pd, scr);
 
 	init_screen(&pd, scr, 16, 16);
	pscr = fromshmptr(struct screen, pd, scr);
 	cc.c = 'H';
 	cc.fg.type = CLR_DEFAULT;
 	cc.bg.type = CLR_DEFAULT;
 	set_cell_screen(pd, *pscr, cc, 1, 1);
 	cc.c = 'i';
 	set_cell_screen(pd, *pscr, cc, 2, 1);

	db = shm_alloc(&pd, sizeof(struct dblbuf));
	pdb = fromshmptr(struct dblbuf, pd, db);
 
 	init_dblbuf(&pd, db, 16, 16);
	pdb = fromshmptr(struct dblbuf, pd, db);
 	cc.c = 'H';
 	set_cell(pd, pdb, cc, 1, 8);
 	cc.c = 'i';
 	set_cell(pd, pdb, cc, 2, 8);
 	dump_dblbuf(&pd, db, STDOUT_FILENO);
	pdb = fromshmptr(struct dblbuf, pd, db);
 	cc.c = '!';
 	set_cell(pd, pdb, cc, 3, 8);
 	dump_dblbuf(&pd, db, STDOUT_FILENO);

	ic = shm_alloc(&pd, sizeof(struct input_ctl));
	pic = fromshmptr(struct input_ctl, pd, ic);
 
 	init_input_ctl(pic);
 	evt = get_input_ctl(pd, pic);
 	evt.type = IT_KEY;
 	evt.data.key.raw = 'A';
 	evt.data.key.parsed = 'a';
 	evt.data.key.mods = IM_SHIFT;
 	add_input_ctl(&pd, ic, evt);
	pic = fromshmptr(struct input_ctl, pd, ic);
 	evt.data.key.raw = 'B';
 	evt.data.key.parsed = 'b';
 	add_input_ctl(&pd, ic, evt);
	pic = fromshmptr(struct input_ctl, pd, ic);
 	evt = get_input_ctl(pd, pic);
 	evt = get_input_ctl(pd, pic);

	it = shm_alloc(&pd, sizeof(struct input_translator));
	pit = fromshmptr(struct input_translator, pd, it);
 
 	init_input_translator(pit);
	input_translator_set_target(pd, it, ic);
 	while(1) {
 		run_input_translator(&pd, it, STDIN_FILENO, 100);
		pit = fromshmptr(struct input_translator, pd, it);
 		while((evt = get_input_ctl(pd, fromshmptr(struct input_ctl, pd, pit->ic))).type != IT_NONE) {
 			if(evt.type == IT_KEY) {
 				printf("Key %c recognized as %c with flags %d\n", evt.data.key.raw,
 						evt.data.key.parsed, (int)evt.data.key.mods);
 			} else {
 				printf("Key %s recognized as special %d with flags %d\n", evt.data.special.raw,
 						(int)evt.data.special.parsed, (int)evt.data.special.mods);
 			}
 			if(evt.data.key.parsed == 'q' && evt.data.key.mods == IM_SHIFT) {
				goto cleanup;
 			}
 		}
 	}
cleanup:
 
	free_shm_allocator(pd, 1);
	return 0;
}

