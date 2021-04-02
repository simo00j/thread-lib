#include "thread.h"
#include "debug.h"

extern thread_t thread_self(void) {
	error("Trying to access the current thread: %s", "not implemented")

	return 0; //TODO: implement
}

extern int thread_create(thread_t *new_thread, void *(*func)(void *), void *func_arg) {
	error("Trying to create a thread: %s", "not implemented");

	return -1; //TODO: implement
}

extern int thread_yield(void) {
	error("Trying to yield: %s", "not implemented")

	return -1; //TODO: implement
}

extern int thread_join(thread_t thread, void **return_value) {
	error("Trying to join a thread: %s", "not implemented")

	return -1; //TODO: implement
}

extern void thread_exit(void *return_value) {
	error("Trying to exit from a thread: %s", "not implemented")

	//TODO: implement
}
