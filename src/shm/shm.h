#ifndef SHM_H
#define SHM_H

#define SHM_ENV_LENGTH 8
#include <unistd.h>

#include "util.h"

#define SHMNULL 0

typedef data_len shmptr;
struct shm_data {
	char shm_name[SHM_ENV_LENGTH+1];
	void *addr;
	size_t size;

	int fd;
};

int init_shm_data(struct shm_data *data, void *addr, int is_parent);
int free_shm_data(struct shm_data *data, int is_parent);
int shm_map_memory(struct shm_data *data, size_t size, int resize);
void shm_unmap_memory(struct shm_data data);
void *from_shmptr(struct shm_data data, shmptr ptr);
shmptr to_shmptr(struct shm_data data, void *ptr);
int shm_is_parent();

#endif

