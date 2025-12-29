#include "shm/allocator.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

struct globals {
	int some_size;
	shmptr the_string;
};

int main(int argc, char **argv) {
	int is_parent;
	struct shm_allocator_pdata pd;
	char c;
	pid_t child;
	struct globals *g;
	char *long_string;
	int i;

	is_parent = argc == 1;
	init_shm_allocator(&pd, NULL, is_parent, sizeof(struct globals));
	g = from_shmptr(pd.shm, shm_first_used(&pd));
	if(is_parent) {
		scanf("%c", &c);
		scanf("%d", &g->some_size);
		long_string = from_shmptr(pd.shm, shm_alloc(&pd, g->some_size + 1));

		g = from_shmptr(pd.shm, shm_first_used(&pd));

		for(i = 0; i < g->some_size; ++i) {
			long_string[i] = c;
		}
		long_string[g->some_size] = '\0';
		g->the_string = to_shmptr(pd.shm, long_string);
		if((child = fork()) == 0) {
			execl(argv[0], argv[0], "second arg!", NULL);
		}
		waitpid(child, 0, 0);
	} else {
		long_string = from_shmptr(pd.shm, g->the_string);
		printf("%s", long_string);
	}
}

