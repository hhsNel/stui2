#include "base/iobuffer.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

#define BUF_ALLOC_GRAN 4096

static int resize_io_buffer(struct shm_allocator_pdata *pd, shmptr_of(struct io_buffer) buf, data_len additional);

void
init_io_buffer(struct io_buffer *buf)
{
	buf->buf = SHMNULL;
	buf->len = buf->cap = 0;
}

void
free_io_buffer(struct shm_allocator_pdata *pd, shmptr_of(struct io_buffer) buf)
{
	struct io_buffer *pbuf;

	shm_access(pd);
	pbuf = fromshmptr(struct io_buffer, *pd, buf);

	shm_free(pd, pbuf->buf);
	pbuf = fromshmptr(struct io_buffer, *pd, buf);

	pbuf->buf = SHMNULL;
	pbuf->len = pbuf->cap = 0;

	shm_leave(pd);
}

int
write_io_buffer(struct shm_allocator_pdata *pd, shmptr_of(struct io_buffer) buf, char *data, data_len len)
{
	char *ptr;
	struct io_buffer *pbuf;

	shm_access(pd);

	if(resize_io_buffer(pd, buf, len) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}

	pbuf = fromshmptr(struct io_buffer, *pd, buf);
	ptr = fromshmptr(char, *pd, pbuf->buf);
	memcpy(ptr + pbuf->len, data, len);
	pbuf->len += len;

	shm_leave(pd);
	return STUI_OK;
};

int
printf_io_buffer(struct shm_allocator_pdata *pd, shmptr_of(struct io_buffer) buf, char *format, ...)
{
	va_list args, counter;
	int needed;
	char *ptr;
	struct io_buffer *pbuf;

	shm_access(pd);
	va_start(args, format);

	va_copy(counter, args);
	needed = vsnprintf(NULL, 0, format, counter);
	if(resize_io_buffer(pd, buf, needed) != STUI_OK) {
		shm_leave(pd);
		return STUI_ERR;
	}
	va_end(counter);

	pbuf = fromshmptr(struct io_buffer, *pd, buf);
	ptr = fromshmptr(char, *pd, pbuf->buf);
	vsprintf(ptr + pbuf->len, format, args);
	va_end(args);

	pbuf->len += needed;

	shm_leave(pd);
	return STUI_OK;
}

int
append_io_buffer(struct shm_allocator_pdata *pd, shmptr_of(struct io_buffer) buf, char *str)
{
	return write_io_buffer(pd, buf, str, strlen(str));
}

int
dump_io_buffer(struct shm_allocator_pdata pd, struct io_buffer *buf, int fd)
{
	data_len written;
	char *ptr;

	ptr = fromshmptr(char, pd, buf->buf);
	while(buf->len > 0) {
		written = write(fd, ptr, buf->len);
		if(written <= 0) {
			return STUI_ERR;
		}
		buf->len -= written;
		ptr += written;
	}
	return STUI_OK;
}

static int
resize_io_buffer(struct shm_allocator_pdata *pd, shmptr_of(struct io_buffer) buf, data_len additional)
{
	shmptr_of(char) ptr;
	data_len new_cap;
	struct io_buffer *pbuf;

	shm_access(pd);

	pbuf = fromshmptr(struct io_buffer, *pd, buf);
	if(pbuf->len + additional > pbuf->cap) {
		new_cap = pbuf->len + ((additional + BUF_ALLOC_GRAN - 1) / BUF_ALLOC_GRAN) * BUF_ALLOC_GRAN;
		ptr = shm_realloc(pd, pbuf->buf, new_cap);
		pbuf = fromshmptr(struct io_buffer, *pd, buf);
		if(ptr == SHMNULL) {
			shm_leave(pd);
			return STUI_ERR;
		}

		pbuf->cap = new_cap;
		pbuf->buf = ptr;
	}

	shm_leave(pd);
	return STUI_OK;
}

