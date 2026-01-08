#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <unistd.h>
#include <semaphore.h>

#include "shm/shm.h"

#define shmptr_of(TYPE) shmptr
#define fromshmptr(PTRTYPE, PDATA, PTR) ((PTRTYPE *)from_shmptr((PDATA).shm, PTR))
#define toshmptr(PDATA, PTR) (to_shmptr(((PDATA).shm), PTR))
#define shmcontainer_of(shmptr, type, member) ({                      \
        const shmptr_of(typeof( ((type *)0)->member )) __mshmptr = (shmptr);    \
        (shmptr_of(type))((shmptr_of(char))__mshmptr - offsetof(type,member));})

struct shm_allocator {
	size_t mapped_size;
	sem_t sem;
};

struct shm_chunk {
	size_t used, free;
}; /* after this struct, there are <used> used bytes, and <free> free bytes, then the next struct shm_chunk */

struct shm_allocator_pdata {
	struct shm_data shm;
	unsigned int accessing;
};

int    init_shm_allocator(struct shm_allocator_pdata *pdata, void *addr, int is_parent, data_len first_chunk_size);
int    free_shm_allocator(struct shm_allocator_pdata  pdata, int is_parent);
shmptr shm_first_used    (struct shm_allocator_pdata *pdata);
void   shm_access        (struct shm_allocator_pdata *pdata);
void   shm_leave         (struct shm_allocator_pdata *pdata);
shmptr shm_alloc         (struct shm_allocator_pdata *pdata, data_len size);
void   shm_free          (struct shm_allocator_pdata *pdata, shmptr ptr);
shmptr shm_realloc       (struct shm_allocator_pdata *pdata, shmptr ptr, data_len size);

#endif

