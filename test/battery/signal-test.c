#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include "thread.h"

#define TESTED_SIGNAL SIGUSR1

int done = 0;

static void handler(int signum) {
	printf("traitement du signal %d\n", signum);
	done = 1;
}

static void *thfunc(void *dummy __attribute__((unused))) {
	thread_signal(TESTED_SIGNAL, handler);
	while (!done) {
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

#ifdef USE_PTHREAD
	// pthread needs to send the signal *after* the receiver registers it
	sleep(1);
#endif
	thread_kill(th, TESTED_SIGNAL);
	thread_join(th, &res);

	return 0; /* unreachable, shut up the compiler */
}
