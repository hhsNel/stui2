#include "shm/allocator.h"

#include <semaphore.h>

#define SHMA(PDATA) ((struct shm_allocator *)((PDATA).shm.addr))
#define SHM_GRAN 4096

static void converge_chunks(struct shm_allocator_pdata *pdata);

int
init_shm_allocator(struct shm_allocator_pdata *pdata, void *addr, int is_parent, data_len first_chunk_size)
{
	struct shm_chunk *first_chunk;

	pdata->accessing = 0;
	if(init_shm_data(&pdata->shm, addr, is_parent) != STUI_OK) return STUI_ERR;

	if(shm_map_memory(&pdata->shm, SHM_GRAN) != STUI_OK) return STUI_ERR;

	if(is_parent) {
		if(first_chunk_size + sizeof(struct shm_allocator) + sizeof(struct shm_chunk) > SHM_GRAN) {
			return STUI_ERR;
		}

		SHMA(*pdata)->mapped_size = SHM_GRAN;
		if(sem_init(&SHMA(*pdata)->sem, 1, 1) != 0) {
			return STUI_ERR;
		}

		*(void **)&first_chunk = pdata->shm.addr + sizeof(struct shm_allocator);
		first_chunk->used = first_chunk_size;
		first_chunk->free =  SHM_GRAN - sizeof(struct shm_allocator) - sizeof(struct shm_chunk);
	} else {
		if(SHMA(*pdata)->mapped_size != SHM_GRAN) {
			if(shm_map_memory(&pdata->shm, SHMA(*pdata)->mapped_size - SHM_GRAN) != STUI_OK) return STUI_ERR;
		}
	}

	return STUI_OK;
}

shmptr
shm_first_used(struct shm_allocator_pdata *pdata)
{
	return sizeof(struct shm_allocator) + sizeof(struct shm_chunk);
}

void
shm_access(struct shm_allocator_pdata *pdata)
{
	if(pdata->accessing) return;
	sem_wait(&SHMA(*pdata)->sem);
	pdata->accessing = 1;
}

void
shm_leave(struct shm_allocator_pdata *pdata)
{
	if(!pdata->accessing) return;
	sem_post(&SHMA(*pdata)->sem);
	pdata->accessing = 0;
}

shmptr
shm_alloc(struct shm_allocator_pdata *pdata, data_len size)
{
	shmptr new;
	struct shm_chunk *pit;
	struct shm_chunk *pnew;
	shmptr traversed;
	data_len this_size, next_size;

	if(size <= 0) {
		return SHMNULL;
	}

	traversed = sizeof(struct shm_allocator);
	pit = from_shmptr(pdata->shm, traversed);
	while(pit->free < size + sizeof(struct shm_chunk)) {
		this_size = sizeof(struct shm_chunk) + pit->used + pit->free; 
		if(traversed + this_size >= pdata->shm.size) {
			next_size = ((size - pit->free + sizeof(struct shm_chunk)+ SHM_GRAN - 1) / SHM_GRAN) * SHM_GRAN;
			if(shm_map_memory(&pdata->shm, next_size) != STUI_OK) {
				return SHMNULL;
			}
			pit = from_shmptr(pdata->shm, traversed);
			SHMA(*pdata)->mapped_size += next_size;
			pit->free += next_size;
			continue;
		}

		traversed += this_size;
		pit = from_shmptr(pdata->shm, traversed);
	}

	new = traversed + sizeof(struct shm_chunk) + pit->used;
	pnew = from_shmptr(pdata->shm, new);
	pnew->used = size;
	pnew->free = pit->free - size - sizeof(struct shm_chunk);
	pit->free = 0;

	return new + sizeof(struct shm_chunk);
}

void
shm_free(struct shm_allocator_pdata *pdata, shmptr ptr)
{
	struct shm_chunk *container;

	container = pdata->shm.addr + ptr - sizeof(struct shm_chunk);
	container->free += container->used;
	container->used = 0;

	converge_chunks(pdata);
}

shmptr
shm_realloc(struct shm_allocator_pdata *pdata, shmptr ptr, data_len size)
{
	return SHMNULL; /* todo */
}

static void
converge_chunks(struct shm_allocator_pdata *pdata)
{
	size_t traversed, this_size;
	struct shm_chunk *this, *next;

	traversed = sizeof(struct shm_allocator);

	while(traversed < pdata->shm.size) {
		*(void **)&this = pdata->shm.addr + traversed;

		this_size = sizeof(struct shm_data) + this->used + this->free;
		if(traversed + this_size >= pdata->shm.size) {
			break;
		}
		*(void **)&next = (void *)this + this_size;
		traversed += this_size;
		if(next->used == 0) {
			this_size = sizeof(struct shm_chunk) + next->free;
			this->free += this_size;
			traversed += this_size;
		}
	}
}

