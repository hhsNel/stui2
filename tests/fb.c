#include "agfx/agfxplane.h"
#include "agfx/fbctl.h"
#include "shm/allocator.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	struct shm_allocator_pdata pd;
	shmptr_of(struct fb_ctl) fc;
	shmptr_of(struct agfx_plane) ap;
	struct agfx_plane *pap;
	unsigned int i, j, time;
	struct agfx_pixel pix;

	if(!is_fb_available()) {
		puts("fb is not available\n");
		exit(1);
	}

	init_shm_allocator(&pd, NULL, 1, 0);

	fc = shm_alloc(&pd, sizeof(struct fb_ctl));

	init_fb_ctl(&pd, fc);

	ap = get_fb_agfx_plane(&pd, fc);
	pap = fromshmptr(struct agfx_plane, pd, ap);

	for(time = 0; time < 256; time += 4) {
		for(j = 0; j < 256; ++j) {
			for(i = 0; i < 256; ++i) {
				pix.red = time * 65536 / 256;
				pix.green = i * 65536 / 256;
				pix.blue = j * 65536 / 256;

				set_px_agfx_plane(pd, *pap, pix, i, j);
			}
			flush_fb_ctl(&pd, fc);
			pap = fromshmptr(struct agfx_plane, pd, ap);
		}
	}

	free_fb_ctl(&pd, fc);
	free_shm_allocator(pd, 1);
}

