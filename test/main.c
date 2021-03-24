#include <thread.h>
#include <stdio.h>
#include "thread-utils.h"
#include "test-utils.h"

int main() {
	print_thread_implementation();

	thread_t thread = thread_self();
	printf("Current thread: %p\n", thread);

	test_result("test_test", "%i", 1);
	return 0;
}
