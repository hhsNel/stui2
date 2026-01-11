#ifndef INPUTTRANSLATOR_H
#define INPUTTRANSLATOR_H

#include "base/inputctl.h"

enum input_translator_state {
	ITS_NORMAL,
	ITS_ESC,
	ITS_CSI,
};
struct input_translator {
	enum input_translator_state state;
	struct input_ctl ic;
};

void init_input_translator(struct input_translator *it);
void free_input_translator(struct shm_allocator_pdata *pd, struct input_translator *it);
int   run_input_translator(struct shm_allocator_pdata *pd, struct input_translator *it, int fd);

#endif

