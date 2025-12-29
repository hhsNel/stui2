#include "shm/shm.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SHM_ENV_NAME "STUI_SHM"

int
init_shm_data(struct shm_data *data, void *addr, int is_parent)
{
	char *env;
	unsigned int i;

	data->addr = addr;
	data->size = 0;

	if(is_parent) {
		srand(time(NULL) ^ getpid());
		for(i = 0; i < SHM_ENV_LENGTH; ++i) {
			data->shm_name[i] = 'a' + rand()/((RAND_MAX+1u)/24);
		}
		data->shm_name[SHM_ENV_LENGTH] = '\0';
		if(setenv(SHM_ENV_NAME, data->shm_name, 1) == -1) {
			return STUI_ERR;
		}
	} else {
		env =
#ifdef _GNU_SOURCE
			secure_getenv(SHM_ENV_NAME);
#else
			getenv(SHM_ENV_NAME);
#endif
	
		strncpy(data->shm_name, env, SHM_ENV_LENGTH);
		data->shm_name[SHM_ENV_LENGTH] = '\0';
	}

	data->fd = shm_open(data->shm_name, O_RDWR | O_CREAT,
	                    S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH);
	if(data->fd < 0) {
		return STUI_ERR;
	}

	return STUI_OK;
}

int
shm_map_memory(struct shm_data *data, size_t size)
{
	void *res;

	if(size <= 0) {
		return STUI_ERR;
	}

	if(ftruncate(data->fd, data->size + size) == -1) {
		return STUI_ERR;
	}

	if(data->addr == NULL) {
		res = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, data->fd, 0);
		if(res == MAP_FAILED) {
			return STUI_ERR;
		}
		data->size = size;
	} else {
		res = mremap(data->addr, data->size, data->size + size, MREMAP_MAYMOVE);

		if(res == MAP_FAILED) {
			return STUI_ERR;
		}
		data->size += size;
	}
	data->addr = res;

	return STUI_OK;
}

void
shm_unmap_memory(struct shm_data data)
{
	munmap(data.addr, data.size);
	close(data.fd);
}

void *
from_shmptr(struct shm_data data, shmptr ptr)
{
	return data.addr + ptr;
}

shmptr
to_shmptr(struct shm_data data, void *ptr)
{
	return ptr - data.addr;
}

