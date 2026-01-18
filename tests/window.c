#include "layout/window.h"
#include "elements/dispatch.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void mklabel(struct shm_allocator_pdata *pd, struct element *plabel, ...) {
	va_list args;

	va_start(args, plabel);
	dispatch_init_element(pd, plabel, ELEMENT_LABEL, args);
	va_end(args);
}

int main(int argc, char **argv) {
	struct shm_allocator_pdata pd;
	shmptr_of(struct window) w;
	struct window *pw;
	shmptr_of(struct element) label;
	struct element *plabel;
	struct char_cell cc;

	init_shm_allocator(&pd, NULL, 1, 0);

	w = shm_alloc(&pd, sizeof(struct window));
	pw = fromshmptr(struct window, pd, w);

	init_window(&pd, pw, 32, 32);
	pw = fromshmptr(struct window, pd, w);

	label = shm_alloc(&pd, sizeof(struct element));
	pw = fromshmptr(struct window, pd, w);
	plabel = fromshmptr(struct element, pd, label);
	plabel->z_index = 0;
	plabel->scr_pos.x = 4;
	plabel->scr_pos.y = 6;
	plabel->scr_pos.width = 2;
	plabel->scr_pos.height = 8;
	plabel->render_output = toshmptr(pd, &pw->scr);
	z_index_list_insert(&pd, &pw->elements, 0, label);
	pw = fromshmptr(struct window, pd, w);
	plabel = fromshmptr(struct element, pd, label);

	mklabel(&pd, plabel, cc, "this is some text that is too long");
	pw = fromshmptr(struct window, pd, w);
	plabel = fromshmptr(struct element, pd, label);

	printf("\e[2J\e[H\nfrom main: output (%lu):\n", sizeof(struct screen));
	fwrite(fromshmptr(struct screen, pd, plabel->render_output), 1, sizeof(struct screen), stdout);
	dispatch_element_draw(&pd, plabel);

	free_shm_allocator(pd, 1);
}

