#define STUI2_GLOBAL
#include "stui2.h"

int main(int argc, char **argv) {
	struct char_cell cc;
	stui2_window win;
	stui2_element text;
	stui2_insertable ins;
	struct rect rect;

	ginit_stui2();

	win = gmain_window();
	ins = gwin_get_insertable(win);
	text = gcreate_element(ins, 0, ELEMENT_TEXT, EL_TEXT_NOSCROLL);

	rect.x      = (struct coord){.resizable=0,.fixed=0};
	rect.y      = (struct coord){.resizable=0,.fixed=0};
	rect.width  = (struct coord){.resizable=0,.fixed=64};
	rect.height = (struct coord){.resizable=0,.fixed=64};
	gset_position(text, rect);

	cc.attr = 0;
	cc.fg.type = cc.bg.type = CLR_DEFAULT;
	stui2_text_append(global_stui2, text, cc, "this is ");
	cc.fg.type = CLR_USR;
	cc.fg.data.usr = 6;
	stui2_text_append(global_stui2, text, cc, "a text.\n");
	cc.fg.type = CLR_DEFAULT;
	stui2_text_append(global_stui2, text, cc, "Of the ");
	cc.fg.type = CLR_USR;
	cc.fg.data.usr = 5;
	stui2_text_append(global_stui2, text, cc, "text");
	cc.fg.type = CLR_DEFAULT;
	stui2_text_append(global_stui2, text, cc, " element, ofc\n");
	stui2_text_append(global_stui2, text, cc, "yay it works\n");

	grender(win);

	gflush();

	gexit_stui2();
}

