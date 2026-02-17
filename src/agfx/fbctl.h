#ifndef FBCTL_H
#define FBCTL_H

#include "util.h"
#include "shm/allocator.h"
#include "agfx/agfxplane.h"

#include <linux/fb.h>

struct fb_ctl {
	int fb_fd;
	char *mapped_fb;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	struct agfx_plane input_plane;
};

int  is_fb_available();
int  init_fb_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct fb_ctl) fc);
void free_fb_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct fb_ctl) fc);
shmptr_of(struct agfx_plane) get_fb_agfx_plane(struct shm_allocator_pdata *pd, shmptr_of(struct fb_ctl) fc);
void flush_fb_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct fb_ctl) fc);

#endif

