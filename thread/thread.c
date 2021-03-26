#include "thread.h"

extern thread_t thread_self(void) {
	return 0; //TODO: implement
}

extern int thread_create(thread_t *new_thread, void *(*func)(void *), void *func_arg) {
	return -1; //TODO: implement
}

extern int thread_yield(void) {
	return -1; //TODO: implement
}

extern int thread_join(thread_t thread, void **return_value) {
	return -1; //TODO: implement
}

extern void thread_exit(void *return_value) {
	//TODO: implement
}
