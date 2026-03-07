#include "agfx/agfxplane.h"
#include "agfx/drmctl.h"
#include "shm/allocator.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	struct shm_allocator_pdata pd;
	shmptr_of(struct drm_ctl) dc;
	shmptr_of(struct agfx_plane) ap;
	struct agfx_plane *pap;
	unsigned int i, j, time;
	struct agfx_pixel pix;

	if(!is_drm_available()) {
		puts("drm is not available\n");
		exit(1);
	}

	init_shm_allocator(&pd, NULL, 1, 0);

	dc = shm_alloc(&pd, sizeof(struct drm_ctl));

	init_drm_ctl(&pd, dc);

	ap = get_drm_agfx_plane(&pd, dc);
	pap = fromshmptr(struct agfx_plane, pd, ap);

	for(time = 0; time < 256; time += 4) {
		for(j = 0; j < 256; ++j) {
			for(i = 0; i < 256; ++i) {
				pix.red = time * 65536 / 256;
				pix.green = i * 65536 / 256;
				pix.blue = j * 65536 / 256;

				set_px_agfx_plane(pd, *pap, pix, i, j);
			}
			flush_drm_ctl(&pd, dc);
			pap = fromshmptr(struct agfx_plane, pd, ap);
		}
	}

	free_drm_ctl(&pd, dc);
	free_shm_allocator(pd, 1);
}

