#include "drmctl.h"

#include <string.h>
#include <stdlib.h>

static int get_mode_resources(int fd, struct drm_mode_card_res *res, uint32_t **connectors);
static int find_connector(int fd, struct drm_mode_card_res *res, struct drm_mode_modeinfo *mode, uint32_t *encoder_id, uint32_t *connector_id);
static int get_encoder(int fd, uint32_t encoder_id, struct drm_mode_get_encoder *enc);
static int create_dumb_buffer(int fd, uint32_t width, uint32_t height, uint32_t *handle, uint32_t *line_length, uint64_t *size, uint32_t *fb_id, char **map, uint64_t *offset);
static int save_crtc(int fd, uint32_t crtc_id, struct drm_mode_crtc *prev_crtc);
static int reset_crtc(int fd, struct drm_mode_crtc prev_crtc, uint32_t prev_connector_id);
static int assign_crtc(int fd, uint32_t crtc_id, uint32_t fb_id, uint32_t connector_id, struct drm_mode_modeinfo mode, struct drm_mode_crtc *crtc);

static char *devices[] = {
	"/dev/dri/card1",
	"/dev/dri/card0",
	"/dev/dri/card2",
	NULL,
};
static int check_device(char *name, int *fd, struct drm_mode_card_res *res, struct drm_mode_modeinfo *mode, uint32_t *encoder_id, uint32_t *connector_id);
static int check_devices(char **names, int *fd, struct drm_mode_card_res *res, struct drm_mode_modeinfo *mode, uint32_t *encoder_id, uint32_t *connector_id);

int
is_drm_available()
{
	int fd;
	struct drm_mode_card_res res;
	struct drm_mode_modeinfo mode;
	uint32_t encoder_id;
	uint32_t connector_id;

	if(check_devices(devices, &fd, &res, &mode, &encoder_id, &connector_id)) {
		if(close(fd) < 0){
			return 0;
		}

		return 1;
	} else {
		return 0;
	}
}

int
init_drm_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct drm_ctl) dc)
{
	struct drm_ctl *pdc;
	uint32_t encoder_id;
	uint32_t connector_id;
	struct drm_mode_get_encoder enc;
	struct agfx_channel r, g, b;
	struct agfx_pixel def;
	struct drm_mode_destroy_dumb dreq;

	shm_access(pd);
	pdc = fromshmptr(struct drm_ctl, *pd, dc);
	pdc->fd = -1;

	if(! check_devices(devices, &pdc->fd, &pdc->res, &pdc->mode, &encoder_id, &connector_id)) {
		shm_leave(pd);
		return STUI_ERR;
	}

	if(! get_encoder(pdc->fd, encoder_id, &enc)) {
		goto cleanup_close;
	}

	pdc->prev_connector_id = connector_id;
	if(! save_crtc(pdc->fd, enc.crtc_id, &pdc->prev_crtc)) {
		goto cleanup_close;
	}

	if(! create_dumb_buffer(pdc->fd, pdc->mode.hdisplay, pdc->mode.vdisplay, &pdc->handle, &pdc->line_length, &pdc->size, &pdc->fb_id, &pdc->mapped_fb, &pdc->mapped_offset)) {
		goto cleanup_destroy_dumb;
	}

	if(ioctl(pdc->fd, DRM_IOCTL_SET_MASTER, 0) < 0) {
		goto cleanup_destroy_dumb;
	}

	if(! assign_crtc(pdc->fd, enc.crtc_id, pdc->fb_id, connector_id, pdc->mode, &pdc->crtc)) {
		goto cleanup_drop_master;
	}

	r.bits      = 8;
	r.offset    = 16;
	r.msb_right = 0;
	g.bits      = 8;
	g.offset    = 8;
	g.msb_right = 0;
	b.bits      = 8;
	b.offset    = 0;
	b.msb_right = 0;
	def.red   = 0;
	def.green = 0;
	def.blue  = 0;
	if(init_agfx_plane(pd, toshmptr(*pd, &pdc->input_plane),
					r, g, b,
					4,
					pdc->line_length,
					pdc->mode.hdisplay,
					pdc->mode.vdisplay,
					def) != STUI_OK) {
		goto cleanup_drop_master;
	}
	pdc = fromshmptr(struct drm_ctl, *pd, dc);

	shm_leave(pd);
	return STUI_OK;

cleanup_drop_master:
	if(ioctl(pdc->fd, DRM_IOCTL_DROP_MASTER, 0) < 0) {
		return STUI_ERR;
	}
cleanup_destroy_dumb:
	if(ioctl(pdc->fd, DRM_IOCTL_MODE_RMFB, &pdc->fb_id) < 0) {
		return STUI_ERR;
	}
	memset(&dreq, 0, sizeof(struct drm_mode_destroy_dumb));
	dreq.handle = pdc->handle;
	if(ioctl(pdc->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq) < 0) {
		return STUI_ERR;
	}
cleanup_close:
	if(close(pdc->fd) < 0) {
		return STUI_ERR;
	}
	return STUI_ERR;
}

void
free_drm_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct drm_ctl) dc)
{
	struct drm_ctl *pdc;
	struct drm_mode_destroy_dumb destroyreq;

	shm_access(pd);
	pdc = fromshmptr(struct drm_ctl, *pd, dc);

	free_agfx_plane(pd, toshmptr(*pd, &pdc->input_plane));
	pdc = fromshmptr(struct drm_ctl, *pd, dc);

	munmap(pdc->mapped_fb, pdc->size);
	if(ioctl(pdc->fd, DRM_IOCTL_MODE_RMFB, &pdc->fb_id) < 0) {
		shm_leave(pd);
		return;
	}

	memset(&destroyreq, 0, sizeof(struct drm_mode_destroy_dumb));
	destroyreq.handle = pdc->handle;
	if(ioctl(pdc->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroyreq) < 0) {
		shm_leave(pd);
		return;
	}

	if(! reset_crtc(pdc->fd, pdc->prev_crtc, pdc->prev_connector_id)) {
		shm_leave(pd);
		return;
	}

	if(ioctl(pdc->fd, DRM_IOCTL_DROP_MASTER, 0) < 0) {
		shm_leave(pd);
		return;
	}

	if(close(pdc->fd) < 0) {
		shm_leave(pd);
		return;
	}

	shm_leave(pd);
}

shmptr_of(struct agfx_plane)
	get_drm_agfx_plane(struct shm_allocator_pdata *pd, shmptr_of(struct drm_ctl) dc)
{
	struct drm_ctl *pdc;

	pdc = fromshmptr(struct drm_ctl, *pd, dc);
	return toshmptr(*pd, &pdc->input_plane);
}

void
flush_drm_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct drm_ctl) dc)
{
	struct drm_ctl *pdc;

	pdc = fromshmptr(struct drm_ctl, *pd, dc);

	memcpy(pdc->mapped_fb, fromshmptr(char, *pd, pdc->input_plane.data), pdc->size);
}

static int
get_mode_resources(int fd, struct drm_mode_card_res *res, uint32_t **connectors)
{
	uint32_t *fbs;
	uint32_t *crtcs;
	uint32_t *encoders;

	memset(res, 0, sizeof(struct drm_mode_card_res));
	if(ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, res) < 0) {
		return 0;
	}

	if(res->count_connectors > 0) {
		*connectors = malloc(res->count_connectors * sizeof(uint32_t));
		if(! *connectors) {
			return 0;
		}
	}
	if(res->count_fbs > 0) {
		fbs = malloc(res->count_fbs * sizeof(uint32_t));
		if(! fbs) {
			if(*connectors) free(*connectors);
			return 0;
		}
	} else {
		fbs = NULL;
	}
	if(res->count_crtcs > 0) {
		crtcs = malloc(res->count_crtcs * sizeof(uint32_t));
		if(! crtcs) {
			if(*connectors) free(*connectors);
			if(fbs) free(fbs);
			return 0;
		}
	} else {
		crtcs = NULL;
	}
	if(res->count_encoders > 0) {
		encoders = malloc(res->count_encoders * sizeof(uint32_t));
		if(! encoders) {
			if(*connectors) free(*connectors);
			if(fbs) free(fbs);
			if(encoders) free(encoders);
			return 0;
		}
	} else {
		encoders = NULL;
	}

	res->connector_id_ptr = (uint64_t)*connectors;
	res->fb_id_ptr        = (uint64_t)fbs;
	res->crtc_id_ptr      = (uint64_t)crtcs;
	res->encoder_id_ptr   = (uint64_t)encoders;

	if(ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, res) < 0) {
		if(*connectors) free(*connectors);
		if(fbs) free(fbs);
		if(crtcs) free(crtcs);
		if(encoders) free(encoders);
		return 0;
	}

	if(fbs) free(fbs);
	if(crtcs) free(crtcs);
	if(encoders) free(encoders);
	return 1;
}

static int
find_connector(int fd, struct drm_mode_card_res *res, struct drm_mode_modeinfo *mode, uint32_t *encoder_id, uint32_t *connector_id)
{
	uint32_t *connectors;
	unsigned int i;
	struct drm_mode_get_connector conn;
	struct drm_mode_modeinfo *modes;
	uint32_t *props;
	uint64_t *prop_values;
	uint32_t *encoders;

	if(get_mode_resources(fd, res, &connectors) == 0) {
		return 0;
	}

	*connector_id = 0;
	for(i = 0; i < res->count_connectors; ++i) {
		memset(&conn, 0, sizeof(struct drm_mode_get_connector));
		conn.connector_id = connectors[i];

		if(ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) < 0) {
			continue;
		}
		/* magic number 1 means connected */
		if(conn.connection != 1 || conn.count_modes == 0) {
			continue;
		}

		modes = NULL;
		if(conn.count_modes > 0) {
			modes = malloc(conn.count_modes * sizeof(struct drm_mode_modeinfo));
			if(! modes) {
				continue;
			}
		}
		props = NULL;
		prop_values = NULL;
		if(conn.count_props > 0) {
			props = malloc(conn.count_props * sizeof(uint32_t));
			if(! props) {
				if(modes) free(modes);
				continue;
			}
			prop_values = malloc(conn.count_props * sizeof(uint64_t));
			if(! props) {
				if(modes) free(modes);
				free(props);
				continue;
			}
		}
		encoders = NULL;
		if(conn.count_encoders > 0) {
			encoders = malloc(conn.count_encoders * sizeof(uint32_t));
			if(! encoders) {
				if(modes) free(modes);
				if(props) {
					free(props);
					free(prop_values);
				}
				continue;
			}
		}

		conn.modes_ptr       = (uint64_t)modes;
		conn.props_ptr       = (uint64_t)props;
		conn.prop_values_ptr = (uint64_t)prop_values;
		conn.encoders_ptr    = (uint64_t)encoders;

		if(ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) < 0) {
			if(modes) free(modes);
			if(props) {
				free(props);
				free(prop_values);
			}
			if(encoders) free(encoders);
			continue;
		}

		*connector_id = conn.connector_id;
		*mode = modes[0];
		*encoder_id = conn.encoder_id;
		if(modes) free(modes);
		if(props) {
			free(props);
			free(prop_values);
		}
		if(encoders) free(encoders);
		break;
	}

	free(connectors);

	if(*connector_id == 0) {
		return 0;
	}

	return 1;
}

static int
get_encoder(int fd, uint32_t encoder_id, struct drm_mode_get_encoder *enc)
{
	memset(enc, 0, sizeof(struct drm_mode_get_encoder));
	enc->encoder_id = encoder_id;
	if(ioctl(fd, DRM_IOCTL_MODE_GETENCODER, enc) < 0) {
		return 0;
	}

	return 1;
}

static int
create_dumb_buffer(int fd, uint32_t width, uint32_t height, uint32_t *handle, uint32_t *line_length, uint64_t *size, uint32_t *fb_id, char **map, uint64_t *offset)
{
	struct drm_mode_create_dumb creq;
	struct drm_mode_fb_cmd fbreq;
	struct drm_mode_map_dumb mreq;
	struct drm_mode_destroy_dumb dreq;

	memset(&creq, 0, sizeof(struct drm_mode_create_dumb));
	creq.width  = width;
	creq.height = height;
	/* TODO: support other formats */
	creq.bpp    = 32;

	if(ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0) {
		return 0;
	}
	
	*handle      = creq.handle;
	*line_length = creq.pitch;
	*size        = creq.size;

	memset(&fbreq, 0, sizeof(struct drm_mode_fb_cmd));
	fbreq.width  = width;
	fbreq.height = height;
	fbreq.pitch  = creq.pitch;
	/* TODO part 2 */
	fbreq.bpp    = 32;
	fbreq.depth  = 24;
	fbreq.handle = creq.handle;

	if(ioctl(fd, DRM_IOCTL_MODE_ADDFB, &fbreq) < 0) {
		goto cleanup_destroy;
	}

	*fb_id = fbreq.fb_id;

	memset(&mreq, 0, sizeof(struct drm_mode_map_dumb));
	mreq.handle  = creq.handle;

	if(ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0) {
		goto cleanup_rmfb;
	}

	*offset = mreq.offset;
	*map    = mmap(NULL, creq.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mreq.offset);
	if(*map == MAP_FAILED) {
		goto cleanup_rmfb;
	}

	return 1;

cleanup_rmfb:
	if(ioctl(fd, DRM_IOCTL_MODE_RMFB, &fbreq.fb_id) < 0) {
		return 0;
	}
cleanup_destroy:
	memset(&dreq, 0, sizeof(struct drm_mode_destroy_dumb));
	dreq.handle = creq.handle;
	if(ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq) < 0) {
		return 0;
	}
	return 0;
}

static int
save_crtc(int fd, uint32_t crtc_id, struct drm_mode_crtc *prev_crtc)
{
	memset(prev_crtc, 0, sizeof(struct drm_mode_crtc));
	prev_crtc->crtc_id = crtc_id;
	if(ioctl(fd, DRM_IOCTL_MODE_GETCRTC, prev_crtc) < 0) {
		return 0;
	}

	return 1;
}

static int
reset_crtc(int fd, struct drm_mode_crtc prev_crtc, uint32_t prev_connector_id)
{
	prev_crtc.set_connectors_ptr = (uint64_t)&prev_connector_id;
	prev_crtc.count_connectors   = 1;
	prev_crtc.mode_valid         = 1;
	if(ioctl(fd, DRM_IOCTL_MODE_SETCRTC, &prev_crtc) < 0) {
		return 0;
	}

	return 1;
}

static int
assign_crtc(int fd, uint32_t crtc_id, uint32_t fb_id, uint32_t connector_id, struct drm_mode_modeinfo mode, struct drm_mode_crtc *crtc)
{
	memset(crtc, 0, sizeof(struct drm_mode_crtc));
	crtc->fb_id              = fb_id;
	crtc->set_connectors_ptr = (uint64_t)&connector_id;
	crtc->count_connectors   = 1;
	crtc->crtc_id            = crtc_id;
	crtc->mode               = mode;
	crtc->mode_valid         = 1;

	if(ioctl(fd, DRM_IOCTL_MODE_SETCRTC, crtc) < 0) {
		return 0;
	}

	return 1;
}

static int
check_device(char *name, int *fd, struct drm_mode_card_res *res, struct drm_mode_modeinfo *mode, uint32_t *encoder_id, uint32_t *connector_id)
{
	*fd = open(name, O_RDWR | O_CLOEXEC);
	if(*fd < 0) return 0;

	if(! find_connector(*fd, res, mode, encoder_id, connector_id)) {
		if(close(*fd) < 0) {
			return 0;
		}
		return 0;
	}

	return 1;
}

static int
check_devices(char **names, int *fd, struct drm_mode_card_res *res, struct drm_mode_modeinfo *mode, uint32_t *encoder_id, uint32_t *connector_id)
{
	char **iter;

	for(iter = names; *iter; ++iter) {
		if(check_device(*iter, fd, res, mode, encoder_id, connector_id)) {
			return 1;
		}
	}

	return 0;
}

