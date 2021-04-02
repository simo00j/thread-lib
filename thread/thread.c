#include <malloc.h>
#include <ucontext.h>
#include <sys/queue.h>
#include "thread.h"
#include "debug.h"

//region Structure declaration

struct thread {
	ucontext_t context;
	void *return_value;
	TAILQ_ENTRY(thread) entries;
};

TAILQ_HEAD(thread_queue, thread);
struct thread_queue threads;
struct thread_queue zombies;

__attribute__((unused)) __attribute__((constructor))
static void initialize_threads() {
	TAILQ_INIT(&threads);
	TAILQ_INIT(&zombies);

	// Create the main thread (so it can call thread_self and thread_yield)
	struct thread *main_thread = malloc(sizeof *main_thread);
	main_thread->return_value = NULL;
	TAILQ_INSERT_HEAD(&threads, main_thread, entries);
}

__attribute__((unused)) __attribute__((destructor))
static void free_threads() {
	struct thread *current = TAILQ_FIRST(&threads);
	struct thread *next;

	while (current != NULL) {
		next = TAILQ_NEXT(current, entries);
		free(current);
		current = next;
	}

	current = TAILQ_FIRST(&zombies);

	while (current != NULL) {
		next = TAILQ_NEXT(current, entries);
		free(current);
		current = next;
	}
}

//endregion

static void free_thread(struct thread *thread) {
	free(&thread->context);
	free(thread);
}

extern thread_t thread_self(void) {
	return TAILQ_FIRST(&threads);
}

extern int thread_create(thread_t *new_thread, void *(*func)(void *), void *func_arg) {
	ucontext_t *new_context = malloc(sizeof *new_context);
	getcontext(new_context); // Initialize the context with default values

	new_context->uc_stack.ss_size = 64 * 1024; // TODO: why?
	new_context->uc_stack.ss_sp = malloc(new_context->uc_stack.ss_size);
	new_context->uc_link = NULL;
	makecontext(new_context, (void (*)(void)) func, 1, func_arg);

	struct thread *new = malloc(sizeof *new);
	new->context = *new_context;
	//*new_thread = new;

	TAILQ_INSERT_TAIL(&threads, new, entries);
	return thread_yield();
}

extern int thread_yield(void) {
	if (TAILQ_FIRST(&threads) == TAILQ_LAST(&threads, thread_queue)) {
		// No thread to yield to: there is only one thread
		return 0;
	} else {
		struct thread *current = TAILQ_FIRST(&threads);
		TAILQ_REMOVE(&threads, current, entries);
		TAILQ_INSERT_TAIL(&threads, current, entries);

		struct thread *next = TAILQ_FIRST(&threads);

		swapcontext(&current->context, &next->context);
		return 0;
	}
}

extern int thread_join(thread_t thread, void **return_value) {
	while (1) {
		struct thread *current_zombie;
		TAILQ_FOREACH(current_zombie, &zombies, entries) {
			if (current_zombie == thread) {
				*return_value = current_zombie->return_value;
				free_thread(current_zombie);
				return 0;
			}
		}

		thread_yield();
	}
}

extern void thread_exit(void *return_value) {
	struct thread *current = TAILQ_FIRST(&threads);
	current->return_value = return_value;
	TAILQ_REMOVE(&threads, current, entries);
	TAILQ_INSERT_TAIL(&zombies, current, entries);

	setcontext(&TAILQ_FIRST(&threads)->context);
}
