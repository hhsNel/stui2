#include "iobuffer.h"
#include "screen.h"
#include "dblbuf.h"
#include "inputctl.h"

int main(int argc, char **argv) {
	struct io_buffer buf;
	struct char_cell **scr;
	struct char_cell cc;
	struct dblbuf db;
	struct input_ctl ic;
	struct input_evt evt;

	init_io_buffer(&buf);
	append_io_buffer(&buf, "Hello, World!\n");
	dump_io_buffer(&buf, 1);

	init_screen(&scr, 16, 16);
	cc.c = 'H';
	cc.fg.type = CLR_DEFAULT;
	cc.bg.type = CLR_DEFAULT;
	set_cell_screen(scr, 16, 16, cc, 1, 1);
	cc.c = 'i';
	set_cell_screen(scr, 16, 16, cc, 2, 1);

	init_dblbuf(&db, 16, 16);
	cc.c = 'H';
	set_cell(&db, cc, 1, 8);
	cc.c = 'i';
	set_cell(&db, cc, 2, 8);
	dump_dblbuf(&db, 1);
	cc.c = '!';
	set_cell(&db, cc, 3, 8);
	dump_dblbuf(&db, 1);

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

	return 0;
}

