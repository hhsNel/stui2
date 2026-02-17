#include "fbctl.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

static char *devices[] = {
	"/dev/fb0",
	"/dev/fb1",
	"/dev/fb2",
	NULL,
};

static int check_device(char *name, int *fd, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo);
static int check_devices(char **names, int *fd, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo);

int
is_fb_available()
{
	int fd;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

	if(check_devices(devices, &fd, &vinfo, &finfo)) {
		if(close(fd) < 0) {
			return 0;
		}
		return 1;
	} else {
		return 0;
	}
}

int
init_fb_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct fb_ctl) fc)
{
	struct fb_ctl *pfc;
	struct agfx_channel r, g, b;
	struct agfx_pixel def;

	shm_access(pd);

	pfc = fromshmptr(struct fb_ctl, *pd, fc);

	if(! check_devices(devices, &pfc->fb_fd, &pfc->vinfo, &pfc->finfo)) {
		shm_leave(pd);
		return STUI_ERR;
	}

	r.bits      = pfc->vinfo.red.length;
	r.offset    = pfc->vinfo.red.offset;
	r.msb_right = pfc->vinfo.red.msb_right;
	g.bits      = pfc->vinfo.green.length;
	g.offset    = pfc->vinfo.green.offset;
	g.msb_right = pfc->vinfo.green.msb_right;
	b.bits      = pfc->vinfo.blue.length;
	b.offset    = pfc->vinfo.blue.offset;
	b.msb_right = pfc->vinfo.blue.msb_right;
	def.red = def.green = def.blue = 0;
	if(init_agfx_plane(
						pd,
						toshmptr(*pd, &pfc->input_plane),
						r, g, b,
						pfc->vinfo.bits_per_pixel / 8,
						pfc->finfo.line_length,
						pfc->vinfo.xres,
						pfc->vinfo.yres,
						def
					  ) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}
	pfc = fromshmptr(struct fb_ctl, *pd, fc);

	pfc->mapped_fb = mmap(0, pfc->finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, pfc->fb_fd, 0);
	if(pfc->mapped_fb == MAP_FAILED) {
		shm_leave(pd);
		return STUI_ERR;
	}

	shm_leave(pd);
	return STUI_OK;
}

void
free_fb_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct fb_ctl) fc)
{
	struct fb_ctl *pfc;

	shm_access(pd);
	pfc = fromshmptr(struct fb_ctl, *pd, fc);

	free_agfx_plane(pd, toshmptr(*pd, &pfc->input_plane));
	pfc = fromshmptr(struct fb_ctl, *pd, fc);

	munmap(pfc->mapped_fb, pfc->finfo.smem_len);

	shm_leave(pd);
}

shmptr_of(struct agfx_plane)
get_fb_agfx_plane(struct shm_allocator_pdata *pd, shmptr_of(struct fb_ctl) fc)
{
	struct fb_ctl *pfc;

	pfc = fromshmptr(struct fb_ctl, *pd, fc);
	return toshmptr(*pd, &pfc->input_plane);
}

void
flush_fb_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct fb_ctl) fc)
{
	struct fb_ctl *pfc;

	pfc = fromshmptr(struct fb_ctl, *pd, fc);

	memcpy(pfc->mapped_fb, fromshmptr(char, *pd, pfc->input_plane.data), pfc->finfo.smem_len);
}

static int
check_device(char *name, int *fd, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo)
{
	*fd = open(name, O_RDWR);

	if(*fd < 0) {
		return 0;
	}

	if(ioctl(*fd, FBIOGET_VSCREENINFO, vinfo) < 0) {
		if(close(*fd) < 0) {
			return 0;
		}
		return 0;
	}

	if(ioctl(*fd, FBIOGET_FSCREENINFO, finfo) < 0) {
		if(close(*fd) < 0) {
			return 0;
		}
		return 0;
	}

	/* don't allow subbyte pixels */
	if(vinfo->bits_per_pixel % 8) {
		if(close(*fd) < 0) {
			return 0;
		}
		return 0;
	}

	return 1;
}

static int
check_devices(char **names, int *fd, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo)
{
	char **iter;

	for(iter = names; *iter; ++iter) {
		if(check_device(*iter, fd, vinfo, finfo)) {
			return 1;
		}
	}

	return 0;
}

