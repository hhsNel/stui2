#include "agfxplane.h"

#include <string.h>

static uint16_t reverse_bit_order(uint16_t num);
static uint64_t pack_channel(uint16_t value, struct agfx_channel channel);
static uint64_t prepare_pixel(struct agfx_pixel pixel, struct agfx_plane ap);

int
init_agfx_plane(struct shm_allocator_pdata *pd, shmptr_of(struct agfx_plane) ap, struct agfx_channel r, struct agfx_channel g, struct agfx_channel b, uint8_t pixel_bytes, scrcoord line_length, scrcoord width, scrcoord height, struct agfx_pixel def)
{
	struct agfx_plane *pap;
	shmptr_of(char) data;
	scrcoord i, j;
	char *ptr;
	uint64_t raw_pixel;

	if(pixel_bytes > 8) {
		return STUI_ERR;
	}

	shm_access(pd);

	pap = fromshmptr(struct agfx_plane, *pd, ap);

	pap->red = r;
	pap->green = g;
	pap->blue = b;
	pap->pixel_bytes = pixel_bytes;
	pap->line_length = line_length;
	pap->width = width;
	pap->height = height;

	data = shm_alloc(pd, line_length * height);
	if(data == SHMNULL) {
		shm_leave(pd);
		return STUI_ERR;
	}
	pap = fromshmptr(struct agfx_plane, *pd, ap);
	pap->data = data;

	raw_pixel = prepare_pixel(def, *pap);
	for(i = 0; i < height; ++i) {
		ptr = fromshmptr(char, *pd, data) + i * line_length;
		for(j = 0; j < width; ++j) {
			memcpy(ptr, &raw_pixel, pixel_bytes);
			ptr += pixel_bytes;
		}
	}

	shm_leave(pd);
	return STUI_OK;
}

void
free_agfx_plane(struct shm_allocator_pdata *pd, shmptr_of(struct agfx_plane) ap)
{
	struct agfx_plane *pap;

	shm_access(pd);
	pap = fromshmptr(struct agfx_plane, *pd, ap);

	shm_free(pd, pap->data);

	shm_leave(pd);
}

int
set_px_agfx_plane(struct shm_allocator_pdata pd, struct agfx_plane ap, struct agfx_pixel pixel, scrcoord x, scrcoord y)
{
	uint64_t raw_pixel;
	scrcoord line_location;
	scrcoord row_location;
	char *pdata;

	pdata = fromshmptr(char, pd, ap.data);

	if(x < 0 || y < 0 || x >= ap.width || y >= ap.height) {
		return STUI_ERR;
	}

	line_location = ap.line_length * y;
	row_location = ap.pixel_bytes * x;

	raw_pixel = prepare_pixel(pixel, ap);

	memcpy(pdata + line_location + row_location, &raw_pixel, ap.pixel_bytes);

	return STUI_OK;
}

static uint16_t
reverse_bit_order(uint16_t num)
{
	uint16_t reversed;
	unsigned int i;

	reversed = 0;
	for(i = 0; i < 16; ++i) {
		reversed <<= 1;
		reversed |= (num >> i) & 1;
	}

	return reversed;
}

static uint64_t
pack_channel(uint16_t value, struct agfx_channel channel)
{
	uint64_t packed;

	if(channel.msb_right) {
		value = reverse_bit_order(value);
		packed = value & ((1ULL << channel.bits) - 1);
		/* if msb_right, first flip then take the (host's) least significant bits */
	} else {
		packed = value >> (16 - channel.bits);
		/* otherwise, take the most significant bits */
	}

	packed <<= channel.offset;

	return packed;
}

static uint64_t
prepare_pixel(struct agfx_pixel pixel, struct agfx_plane ap)
{
	uint64_t raw_pixel;

	raw_pixel = 0;
	raw_pixel |= pack_channel(pixel.red, ap.red);
	raw_pixel |= pack_channel(pixel.green, ap.green);
	raw_pixel |= pack_channel(pixel.blue, ap.blue);

	return raw_pixel;
}

