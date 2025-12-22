#ifndef IO_BUFFER_H
#define IO_BUFFER_H

#include "util.h"

struct io_buffer {
	char *buf;
	data_len len, cap;
};

void init_io_buffer  (struct io_buffer *buf);
void free_io_buffer  (struct io_buffer *buf);
int  write_io_buffer (struct io_buffer *buf, char *data, data_len len);
int  printf_io_buffer(struct io_buffer *buf, char *format, ...);
int  append_io_buffer(struct io_buffer *buf, char *str);
int  dump_io_buffer  (struct io_buffer *buf, int fd);

#endif

