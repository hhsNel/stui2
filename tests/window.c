#define STUI2_GLOBAL
#include "stui2.h"
#include "layout/window.h"
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
	struct char_cell cc;
	stui2_window win;
	stui2_element label;
	struct element *plabel;
	stui2_insertable ins;

	ginit_stui2();

	win = gmain_window();
	ins = gwin_get_insertable(win);

	cc.attr = 0;
	cc.fg.type = cc.bg.type = CLR_DEFAULT;
	label = gcreate_element(ins, 0, ELEMENT_LABEL, cc, "this is some text\nthat is too long\nfor this label");

	plabel = fromshmptr(struct element, *(struct shm_allocator_pdata *)global_stui2, label);
	plabel->scr_pos.x = 4;
	plabel->scr_pos.y = 6;
	plabel->scr_pos.width = 4;
	plabel->scr_pos.height = 8;

	grender(win);

	gflush();

	gexit_stui2();
}

