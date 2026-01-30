#ifndef INPUTTRANSLATOR_H
#define INPUTTRANSLATOR_H

#include "base/inputctl.h"
#include "shm/allocator.h"

enum input_translator_state {
	ITS_NORMAL,
	ITS_ESC,
	ITS_CSI,
};
struct input_translator {
	enum input_translator_state state;
	shmptr_of(struct input_ctl) ic;
};

void init_input_translator      (struct input_translator *it);
void input_translator_set_target(struct shm_allocator_pdata pd, shmptr_of(struct input_translator) it, shmptr_of(struct input_ctl) ic);
void free_input_translator      (struct shm_allocator_pdata *pd, shmptr_of(struct input_translator) it);
int  run_input_translator       (struct shm_allocator_pdata *pd, shmptr_of(struct input_translator) it, int fd, int timeout);

#endif

