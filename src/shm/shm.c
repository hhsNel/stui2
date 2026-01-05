#include "shm/shm.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SHM_ENV_NAME "STUI_SHM"
#define SHM_ENV_PREFIX "/stui-"

int
init_shm_data(struct shm_data *data, void *addr, int is_parent)
{
	char *env;
	unsigned int i;
	char full_name[sizeof(SHM_ENV_PREFIX) + SHM_ENV_LENGTH];

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

	sprintf(full_name, "%s%s", SHM_ENV_PREFIX, data->shm_name);
	data->fd = shm_open(full_name, O_RDWR | O_CREAT,
	                    S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH);
	if(data->fd < 0) {
		return STUI_ERR;
	}

	return STUI_OK;
}

int
free_shm_data(struct shm_data *data, int is_parent)
{
	char full_name[sizeof(SHM_ENV_PREFIX) + SHM_ENV_LENGTH];

	if(is_parent) {
		sprintf(full_name, "%s%s", SHM_ENV_PREFIX, data->shm_name);
		if(shm_unlink(full_name) == -1) return STUI_ERR;
	}

	return STUI_OK;
}

int
shm_map_memory(struct shm_data *data, size_t size, int resize)
{
	void *res;

	if(size <= 0) {
		return STUI_ERR;
	}

	if(resize) {
		if(ftruncate(data->fd, data->size + size) == -1) {
			return STUI_ERR;
		}
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
	if(ptr == SHMNULL) {
		return NULL;
	}

	return data.addr + ptr;
}

shmptr
to_shmptr(struct shm_data data, void *ptr)
{
	return ptr - data.addr;
}

int
shm_is_parent()
{
	return
#ifdef _GNU_SOURCE
			secure_getenv(SHM_ENV_NAME)
#else
			getenv(SHM_ENV_NAME)
#endif
			== NULL;
}

