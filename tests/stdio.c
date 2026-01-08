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
 	struct io_buffer buf;
 	struct screen scr;
 	struct char_cell cc;
 	struct dblbuf db;
 	struct input_ctl ic;
 	struct input_evt evt;
 	struct input_translator it;

	init_shm_allocator(&pd, NULL, 1, 0);
 
 	init_io_buffer(&buf);
 	append_io_buffer(&pd, &buf, "Hello, World!\n");
 	dump_io_buffer(pd, &buf, STDOUT_FILENO);
 
 	init_screen(&pd, &scr, 16, 16);
 	cc.c = 'H';
 	cc.fg.type = CLR_DEFAULT;
 	cc.bg.type = CLR_DEFAULT;
 	set_cell_screen(pd, scr, cc, 1, 1);
 	cc.c = 'i';
 	set_cell_screen(pd, scr, cc, 2, 1);
 
 	init_dblbuf(&pd, &db, 16, 16);
 	cc.c = 'H';
 	set_cell(pd, &db, cc, 1, 8);
 	cc.c = 'i';
 	set_cell(pd, &db, cc, 2, 8);
 	dump_dblbuf(&pd, &db, STDOUT_FILENO);
 	cc.c = '!';
 	set_cell(pd, &db, cc, 3, 8);
 	dump_dblbuf(&pd, &db, STDOUT_FILENO);
 
 	init_input_ctl(&ic);
 	evt = get_input_ctl(&ic);
 	evt.type = IT_KEY;
 	evt.data.key.raw = 'A';
 	evt.data.key.parsed = 'a';
 	evt.data.key.mods = IM_SHIFT;
 	add_input_ctl(&ic, evt);
 	evt.data.key.raw = 'B';
 	evt.data.key.parsed = 'b';
 	add_input_ctl(&ic, evt);
 	evt = get_input_ctl(&ic);
 	evt = get_input_ctl(&ic);
 
 	init_input_translator(&it);
 	while(1) {
 		run_input_translator(&it, STDIN_FILENO);
 		while((evt = get_input_ctl(&it.ic)).type != IT_NONE) {
 			if(evt.type == IT_KEY) {
 				printf("Key %c recognized as %c with flags %d\n", evt.data.key.raw,
 						evt.data.key.parsed, (int)evt.data.key.mods);
 			} else {
 				printf("Key %s recognized as special %d with flags %d\n", evt.data.special.raw,
 						(int)evt.data.special.parsed, (int)evt.data.special.mods);
 			}
 			if(evt.data.key.parsed == 'q' && evt.data.key.mods == IM_SHIFT) {
 				exit(0);
 			}
 		}
 		sleep(1);
 	}
 
	free_shm_allocator(pd, 1);
	return 0;
}

