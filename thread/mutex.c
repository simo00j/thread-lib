#include "thread.h"

struct thread_mutex {
	int dummy;
};

int thread_mutex_init(thread_mutex_t *mutex) {
	return -1;
}

int thread_mutex_destroy(thread_mutex_t *mutex) {
	return -1;
}

int thread_mutex_lock(thread_mutex_t *mutex) {
	return -1;
}

int thread_mutex_unlock(thread_mutex_t *mutex) {
	return -1;
}
