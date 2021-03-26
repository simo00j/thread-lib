#include <stdio.h>
#include "thread-utils.h"

void print_thread_implementation() {
#ifndef USE_PTHREAD
	printf("\n\x1b[35m*** Using custom thread implementation ***\x1b[0m\n\n");
#else
	printf("\n\x1b[35m*** Using UNIX pthread implementation ***\x1b[0m\n\n");
#endif
}
