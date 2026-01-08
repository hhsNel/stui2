#include "base/iobuffer.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

#define BUF_ALLOC_GRAN 4096

static int resize_io_buffer(struct shm_allocator_pdata *pd, struct io_buffer *buf, data_len additional);

void
init_io_buffer(struct io_buffer *buf)
{
	buf->buf = SHMNULL;
	buf->len = buf->cap = 0;
};

void
free_io_buffer(struct shm_allocator_pdata *pd, struct io_buffer *buf)
{
	shm_free(pd, buf->buf);
	buf->buf = SHMNULL;
	buf->len = buf->cap = 0;
};

int
write_io_buffer(struct shm_allocator_pdata *pd, struct io_buffer *buf, char *data, data_len len)
{
	char *ptr;

	if(resize_io_buffer(pd, buf, len) != STUI_OK) return STUI_ERR;

	ptr = fromshmptr(char, *pd, buf->buf);
	strncpy(ptr + buf->len, data, len);
	buf->len += len;
	return STUI_OK;
};

int
printf_io_buffer(struct shm_allocator_pdata *pd, struct io_buffer *buf, char *format, ...)
{
	va_list args, counter;
	int needed;
	char *ptr;

	va_start(args, format);

	va_copy(counter, args);
	needed = vsnprintf(NULL, 0, format, counter);
	if(resize_io_buffer(pd, buf, needed) != STUI_OK) {
		return STUI_ERR;
	}
	va_end(counter);

	ptr = fromshmptr(char, *pd, buf->buf);
	vsprintf(ptr + buf->len, format, args);
	va_end(args);

	buf->len += needed;

	return STUI_OK;
}

int
append_io_buffer(struct shm_allocator_pdata *pd, struct io_buffer *buf, char *str)
{
	return write_io_buffer(pd, buf, str, strlen(str));
};

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
};

static int
resize_io_buffer(struct shm_allocator_pdata *pd, struct io_buffer *buf, data_len additional)
{
	shmptr_of(char) ptr;
	data_len new_cap;

	if(buf->len + additional > buf->cap) {
		new_cap = buf->len + ((additional + BUF_ALLOC_GRAN - 1) / BUF_ALLOC_GRAN) * BUF_ALLOC_GRAN;
		ptr = shm_realloc(pd, buf->buf, new_cap);
		if(ptr == SHMNULL) {
			return STUI_ERR;
		}

		buf->cap = new_cap;
		buf->buf = ptr;
	}

	return STUI_OK;
}

