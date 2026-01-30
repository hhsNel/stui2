#define STUI2_GLOBAL
#include "stui2.h"

#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
	struct char_cell cc;
	stui2_window win;
	stui2_element label;
	stui2_insertable ins;
	struct rect rect;
	struct input_evt evt;
	char buff[64];

	ginit_stui2();

	win = gmain_window();
	ins = gwin_get_insertable(win);

	cc.attr = 0;
	cc.fg.type = cc.bg.type = CLR_DEFAULT;
	label = gcreate_element(ins, 0, ELEMENT_LABEL, cc, "");

	rect.x      = (struct coord){.resizable=0                   ,.fixed=0};
	rect.y      = (struct coord){.resizable=0                   ,.fixed=0};
	rect.width  = (struct coord){.resizable=RESIZABLE_COORD_BASE,.fixed=0};
	rect.height = (struct coord){.resizable=RESIZABLE_COORD_BASE,.fixed=0};
	gset_position(label, rect);

	while(1) {
		gget_input(win, 10);
		while((evt = gnext_event(win)).type != IT_NONE) {
			if(evt.type == IT_KEY) {
				snprintf(buff, 63, "the raw event was %c, the parsed was %c. SHIFT*%i | CTRL*%i", evt.data.key.raw, evt.data.key.parsed, (evt.data.key.mods & IM_SHIFT) != 0, (evt.data.key.mods & IM_CONTROL) != 0);
			} else {
				snprintf(buff, 63, "special key detected, parsed: %i. SHIFT*%i | CTRL*%i", evt.data.special.parsed, (evt.data.special.mods & IM_SHIFT) != 0, (evt.data.special.mods & IM_CONTROL) != 0);
			}
			buff[63] = '\0';
			stui2_label_set_string(global_stui2, label, buff);

			grender(win);
			gflush();
			if(evt.type == IT_KEY && evt.data.key.parsed == 'q' && evt.data.key.mods == IM_SHIFT) {
				goto cleanup;
			}
		}
	}
cleanup:

	gexit_stui2();
}

