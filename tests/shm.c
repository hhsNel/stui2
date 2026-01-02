#include "shm/allocator.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

struct globals {
	int max_length;
	shmptr array;
};
#define GPTR (fromshmptr(struct globals, pd, shm_first_used(&pd)))

int main(int argc, char **argv) {
	int is_parent;
	struct shm_allocator_pdata pd;
	char c;
	pid_t child;
	shmptr string;
	int i, j;
	shmptr alloced;

	is_parent = shm_is_parent();
	init_shm_allocator(&pd, NULL, is_parent, sizeof(struct globals));
	if(is_parent) {
		scanf("%c", &c);
		scanf("%d", &GPTR->max_length);
		alloced = shm_alloc(&pd, sizeof(shmptr) * GPTR->max_length);
		GPTR->array = alloced;

		for(i = 0; i < GPTR->max_length; ++i) {
			string = shm_alloc(&pd, i + 1);
			fromshmptr(shmptr, pd, GPTR->array)[i] = string;
			for(j = 0; j < i; ++j) {
				fromshmptr(char, pd, string)[j] = c;
			}
			fromshmptr(char, pd, string)[i] = '\0';
		}
		if((child = fork()) == 0) {
			execl(argv[0], argv[0], NULL);
		}
		waitpid(child, 0, 0);
	} else {
		for(i = 0; i < GPTR->max_length; ++i) {
			string = fromshmptr(shmptr, pd, GPTR->array)[i];
			printf("%s\n", fromshmptr(char, pd, string));
		}
	}
}

