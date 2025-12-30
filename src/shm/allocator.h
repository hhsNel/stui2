#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <unistd.h>
#include <semaphore.h>

#include "shm/shm.h"

#define SHMNULL 0
#define fromshmptr(PTRTYPE, PDATA, PTR) ((PTRTYPE *)from_shmptr((PDATA).shm, PTR))
#define toshmptr(PDATA, PTR) (to_shmptr(&((PDATA).shm), PTR))

struct shm_allocator {
	size_t mapped_size;
	sem_t sem;
};

struct shm_chunk {
	size_t used, free;
}; /* after this struct, there are <used> used bytes, and <free> free bytes, then the next struct shm_chunk */

struct shm_allocator_pdata {
	struct shm_data shm;
	int accessing;
};

int init_shm_allocator(struct shm_allocator_pdata *pdata, void *addr, int is_parent, data_len first_chunk_size);
shmptr shm_first_used(struct shm_allocator_pdata *pdata);
void shm_access(struct shm_allocator_pdata *pdata);
void shm_leave(struct shm_allocator_pdata *pdata);
shmptr shm_alloc(struct shm_allocator_pdata *pdata, data_len size);
void shm_free(struct shm_allocator_pdata *pdata, shmptr ptr);
shmptr shm_realloc(struct shm_allocator_pdata *pdata, shmptr ptr, data_len size);

#endif

