#ifndef INPUTCTL_H
#define INPUTCTL_H

#include <stdint.h>

#include "util.h"
#include "shm/allocator.h"

struct input_ctl {
	shmptr_of(struct input_evt) buf;
	data_len len, cap;
	data_len begin, end;
};

void init_input_ctl(struct input_ctl *ic);
void free_input_ctl(struct shm_allocator_pdata *pd, shmptr_of(struct input_ctl) ic);
int  add_input_ctl (struct shm_allocator_pdata *pd, shmptr_of(struct input_ctl) ic, struct input_evt evt);
struct input_evt get_input_ctl(struct shm_allocator_pdata pd, struct input_ctl *ic);

#endif

