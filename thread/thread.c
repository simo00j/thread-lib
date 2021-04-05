#include <malloc.h>
#include <ucontext.h>
#include <sys/queue.h>
#include <stdlib.h>
#include "thread.h"
#include "debug.h"

//region Structure declaration

struct thread {
	ucontext_t context;
	void *return_value;
#ifdef USE_DEBUG
	short id;
#endif
	TAILQ_ENTRY(thread) entries;
};

#ifdef USE_DEBUG
static short next_thread_id = 0;
#endif

TAILQ_HEAD(thread_queue, thread);
struct thread_queue threads;
struct thread_queue zombies;

static void free_thread(struct thread *thread) {
	debug("%hd is being freed…", thread->id)
	free(thread);
}

__attribute__((unused)) __attribute__((constructor))
static void initialize_threads() {
	TAILQ_INIT(&threads);
	TAILQ_INIT(&zombies);

	// Create the main thread (so it can call thread_self and thread_yield)
	struct thread *main_thread = malloc(sizeof *main_thread);
	main_thread->return_value = NULL;
#ifdef USE_DEBUG
	main_thread->id = next_thread_id++;
#endif

	TAILQ_INSERT_HEAD(&threads, main_thread, entries);
	debug("%hd is the main thread.", main_thread->id)
}

__attribute__((unused)) __attribute__((destructor))
static void free_threads() {
	printf("\n");
	info("%s, now freeing all remaining threads…", "Program has exited")

	struct thread *current = TAILQ_FIRST(&threads);
	struct thread *next;

	while (current != NULL) {
		next = TAILQ_NEXT(current, entries);
		free_thread(current);
		current = next;
	}

	current = TAILQ_FIRST(&zombies);

	while (current != NULL) {
		next = TAILQ_NEXT(current, entries);
		free_thread(current);
		current = next;
	}
}

//endregion

static struct thread *thread_self_safe(void) {
	return TAILQ_FIRST(&threads);
}

extern thread_t thread_self(void) {
	return thread_self_safe();
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
	new->return_value = NULL;
#ifdef USE_DEBUG
	new->id = next_thread_id++;
#endif
	//*new_thread = new;
	debug("%hd was just created.", new->id)

	TAILQ_INSERT_TAIL(&threads, new, entries);
	return thread_yield();
}

static int thread_is_alone(void) {
	return TAILQ_FIRST(&threads) == TAILQ_LAST(&threads, thread_queue);
}

extern int thread_yield(void) {
	if (thread_is_alone()) {
		debug("%hd: No thread to yield to, noop.", thread_self_safe()->id)
		// No thread to yield to: there is only one thread
		return 0;
	} else {
		struct thread *current = TAILQ_FIRST(&threads);
		TAILQ_REMOVE(&threads, current, entries);
		TAILQ_INSERT_TAIL(&threads, current, entries);

		struct thread *next = TAILQ_FIRST(&threads);

		debug("yield: %hd -> %hd", current->id, next->id)
		swapcontext(&current->context, &next->context);
		return 0;
	}
}

extern int thread_join(thread_t thread, void **return_value) {
	struct thread *target = thread;
	debug("%hd: Will join %hd", thread_self_safe()->id, target->id)

	while (1) {
		struct thread *current_zombie;
		TAILQ_FOREACH(current_zombie, &zombies, entries) {
			debug("%hd: Found zombie %hd…", thread_self_safe()->id,
			      current_zombie->id)
			if (current_zombie == target) {
				if (return_value != NULL) // The client wants the return value
					*return_value = current_zombie->return_value;
				free_thread(current_zombie);
				return 0;
			}
		}

		if (thread_is_alone()) {
			error("%hd: Trying to join %hd, yet there is no thread to yield to. This should not happen.",
			      thread_self_safe()->id, target->id)
			exit(1);
		}
		thread_yield();
	}
}

extern void thread_exit(void *return_value) {
	struct thread *current = TAILQ_FIRST(&threads);
	current->return_value = return_value;
	TAILQ_REMOVE(&threads, current, entries);
	TAILQ_INSERT_TAIL(&zombies, current, entries);
	debug("%hd has died with return value %p",
	      current->id, return_value)

	setcontext(&TAILQ_FIRST(&threads)->context);
}
