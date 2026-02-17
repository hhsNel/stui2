#ifndef AGFX_PLANE_H
#define AGFX_PLANE_H

#include "util.h"
#include "shm/allocator.h"

#include <stdint.h>

struct agfx_channel {
	uint8_t bits, offset;
	int8_t msb_right;
};

struct agfx_pixel {
	uint16_t red, green, blue;
};

struct agfx_plane {
	struct agfx_channel red;
	struct agfx_channel green;
	struct agfx_channel blue;
	uint8_t pixel_bytes;
	scrcoord line_length;
	scrcoord width, height;
	shmptr_of(char) data;
};

int  init_agfx_plane(struct shm_allocator_pdata *pd, shmptr_of(struct agfx_plane) ap, struct agfx_channel r, struct agfx_channel g, struct agfx_channel b, uint8_t pixel_bytes, scrcoord line_length, scrcoord width, scrcoord height, struct agfx_pixel def);
void free_agfx_plane(struct shm_allocator_pdata *pd, shmptr_of(struct agfx_plane) ap);
int  set_px_agfx_plane(struct shm_allocator_pdata pd, struct agfx_plane ap, struct agfx_pixel pixel, scrcoord x, scrcoord y);

#endif

