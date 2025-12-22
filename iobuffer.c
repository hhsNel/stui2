#include "iobuffer.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

#define BUF_ALLOC_GRAN 4096

static int resize_io_buffer(struct io_buffer *buf, data_len additional);

void
init_io_buffer(struct io_buffer *buf)
{
	buf->buf = NULL;
	buf->len = buf->cap = 0;
};

void
free_io_buffer(struct io_buffer *buf)
{
	free(buf->buf);
	buf->buf = NULL;
	buf->len = buf->cap = 0;
};

int
write_io_buffer(struct io_buffer *buf, char *data, data_len len)
{
	resize_io_buffer(buf, len);

	strncpy(buf->buf + buf->len, data, len);
	buf->len += len;
	return STUI_OK;
};

int
printf_io_buffer(struct io_buffer *buf, char *format, ...)
{
	va_list args, counter;
	int needed;

	va_start(args, format);

	va_copy(counter, args);
	needed = vsnprintf(NULL, 0, format, counter);
	if(resize_io_buffer(buf, needed) != STUI_OK) {
		return STUI_ERR;
	}
	va_end(counter);

	vsprintf(buf->buf + buf->len, format, args);
	va_end(args);

	buf->len += needed;

	return STUI_OK;
}

int
append_io_buffer(struct io_buffer *buf, char *str)
{
	return write_io_buffer(buf, str, strlen(str));
};

int
dump_io_buffer(struct io_buffer *buf, int fd)
{
	data_len written;
	char *ptr;

	ptr = buf->buf;
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
resize_io_buffer(struct io_buffer *buf, data_len additional)
{
	char *ptr;
	data_len new_cap;

	if(buf->len + additional > buf->cap) {
		new_cap = buf->len + ((additional + BUF_ALLOC_GRAN - 1) / BUF_ALLOC_GRAN) * BUF_ALLOC_GRAN;
		ptr = realloc(buf->buf, buf->cap);
		if(! ptr) {
			return STUI_ERR;
		}

		buf->cap = new_cap;
		buf->buf = ptr;
	}

	return STUI_OK;
}

