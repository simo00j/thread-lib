#include <stdio.h>
#include <assert.h>
#include "thread.h"

int done = 0;

static void handler(int signum){
  printf("traitement du signal %d\n",signum);
  done = 1;
}

static void *thfunc(void *dummy __attribute__((unused))) {
	while(!done){
        thread_yield();
    }
    printf("le thread se termine\n");
	return NULL;
}

int main() {
	thread_t th;
	int err;
    void *res = NULL;

	err = thread_create(&th, thfunc, NULL);
	assert(!err);


    thread_signal(1, handler);

    thread_kill(th, 1);


    thread_join(th, &res);

	return 0; /* unreachable, shut up the compiler */
}
