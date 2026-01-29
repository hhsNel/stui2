#define STUI2_GLOBAL
#include "stui2.h"

int main(int argc, char **argv) {
	struct char_cell cc;
	stui2_window win;
	stui2_element label;
	stui2_insertable ins;
	struct rect rect;

	ginit_stui2();

	win = gmain_window();
	ins = gwin_get_insertable(win);

	cc.attr = 0;
	cc.fg.type = cc.bg.type = CLR_DEFAULT;
	label = gcreate_element(ins, 0, ELEMENT_LABEL, cc, "this is some text\nthat is too long\nfor this label");

	rect.x      = (struct coord){.resizable=0,.fixed=4};
	rect.y      = (struct coord){.resizable=0,.fixed=6};
	rect.width  = (struct coord){.resizable=0,.fixed=4};
	rect.height = (struct coord){.resizable=0,.fixed=8};
	gset_position(label, rect);

	grender(win);

	gflush();

	gexit_stui2();
}

