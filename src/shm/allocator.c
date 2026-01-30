#include "shm/allocator.h"

#include <semaphore.h>
#include <string.h>

#define SHMA(PDATA) ((struct shm_allocator *)((PDATA).shm.addr))
#define SHM_GRAN 4096

static void converge_chunks(struct shm_allocator_pdata *pdata);
static int check_resizes(struct shm_allocator_pdata *pdata);

int
init_shm_allocator(struct shm_allocator_pdata *pdata, void *addr, int is_parent, data_len first_chunk_size)
{
	struct shm_chunk *first_chunk;

	pdata->accessing = 0;
	if(init_shm_data(&pdata->shm, addr, is_parent) != STUI_OK) return STUI_ERR;

	if(is_parent) {
		if(shm_map_memory(&pdata->shm, SHM_GRAN, 1) != STUI_OK) return STUI_ERR;

		if(first_chunk_size + sizeof(struct shm_allocator) + sizeof(struct shm_chunk) > SHM_GRAN) {
			return STUI_ERR;
		}

		SHMA(*pdata)->mapped_size = SHM_GRAN;
		if(sem_init(&SHMA(*pdata)->sem, 1, 1) != 0) {
			return STUI_ERR;
		}

		*(void **)&first_chunk = pdata->shm.addr + sizeof(struct shm_allocator);
		first_chunk->used = first_chunk_size;
		first_chunk->free = SHM_GRAN - first_chunk_size - sizeof(struct shm_allocator) - sizeof(struct shm_chunk);
	} else {
		if(shm_map_memory(&pdata->shm, SHM_GRAN, 0) != STUI_OK) return STUI_ERR;

		if(SHMA(*pdata)->mapped_size != SHM_GRAN) {
			if(shm_map_memory(&pdata->shm, SHMA(*pdata)->mapped_size - SHM_GRAN, 0) != STUI_OK) return STUI_ERR;
		}
	}

	return STUI_OK;
}

int
free_shm_allocator(struct shm_allocator_pdata pdata, int is_parent)
{
	return free_shm_data(pdata.shm, is_parent);
}

shmptr
shm_first_used(struct shm_allocator_pdata *pdata)
{
	if(check_resizes(pdata) != STUI_OK) return SHMNULL;

	return sizeof(struct shm_allocator) + sizeof(struct shm_chunk);
}

void
shm_access(struct shm_allocator_pdata *pdata)
{
	if(check_resizes(pdata) != STUI_OK) return;
	if(pdata->accessing ++) return;
	sem_wait(&SHMA(*pdata)->sem);
}

void
shm_leave(struct shm_allocator_pdata *pdata)
{
	if(check_resizes(pdata) != STUI_OK) return;
	if(--pdata->accessing != 0) return;
	sem_post(&SHMA(*pdata)->sem);
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
	if(check_resizes(pdata) != STUI_OK) return SHMNULL;

	traversed = sizeof(struct shm_allocator);
	pit = from_shmptr(pdata->shm, traversed);
	while(1) {
		this_size = sizeof(struct shm_chunk) + pit->used + pit->free; 
		if(traversed + this_size >= pdata->shm.size && pit->free < sizeof(struct shm_chunk) + size ) {
			next_size = ((size + sizeof(struct shm_chunk) - pit->free + SHM_GRAN - 1) / SHM_GRAN) * SHM_GRAN;
			if(shm_map_memory(&pdata->shm, next_size, 1) != STUI_OK) {
				return SHMNULL;
			}
			pit = from_shmptr(pdata->shm, traversed);
			SHMA(*pdata)->mapped_size += next_size;
			pit->free += next_size;

			continue;
		}

		if(pit->free >= size + sizeof(struct shm_chunk)) break;

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

	if(check_resizes(pdata) != STUI_OK) return;
	container = pdata->shm.addr + ptr - sizeof(struct shm_chunk);
	container->free += container->used;
	container->used = 0;

	converge_chunks(pdata);
}

shmptr
shm_realloc(struct shm_allocator_pdata *pdata, shmptr ptr, data_len size)
{
	struct shm_chunk *container;
	shmptr new_ptr;

	if(ptr == SHMNULL) {
		return shm_alloc(pdata, size);
	}

	if(size <= 0) {
		return SHMNULL;
	}
	if(check_resizes(pdata) != STUI_OK) return SHMNULL;

	container = fromshmptr(struct shm_chunk, *pdata, ptr - sizeof(struct shm_chunk));
	if(container->used + container->free >= size) {
		container->free += container->used - size;
		container->used = size;
		return ptr;
	}

	new_ptr = shm_alloc(pdata, size);
	if(new_ptr == SHMNULL) {
		return SHMNULL;
	}
	container = fromshmptr(struct shm_chunk, *pdata, ptr - sizeof(struct shm_chunk));

	memcpy(fromshmptr(char, *pdata, new_ptr), fromshmptr(char, *pdata, ptr), MIN(size, container->used));

	shm_free(pdata, ptr);
	return new_ptr;
}

static void
converge_chunks(struct shm_allocator_pdata *pdata)
{
	size_t traversed, this_size;
	struct shm_chunk *this, *next;

	if(check_resizes(pdata) != STUI_OK) return;

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

static int
check_resizes(struct shm_allocator_pdata *pdata)
{
	if(SHMA(*pdata)->mapped_size != pdata->shm.size) {
		return shm_map_memory(&pdata->shm, SHMA(*pdata)->mapped_size - pdata->shm.size, 0);
	}

	return STUI_OK;
}

