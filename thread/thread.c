#include <stdio.h>
#include <ucontext.h>
#include <sys/queue.h>
#include <stdlib.h>
#include "thread.h"
#include <valgrind/valgrind.h>

#include "debug.h"
#include <assert.h>

#define STACK_SIZE (64 * 1024)

#ifdef USE_DEBUG
static short next_thread_id = 0;
#endif

//region Structure declaration

struct thread {
	ucontext_t context;
	void *return_value;
	//void *stack;
#ifdef USE_DEBUG
	short id;
#endif
	unsigned int valgrind_stack;
	char is_zombie; //0 = zombie, 1 = active
	TAILQ_ENTRY(thread) entries;
};

TAILQ_HEAD(thread_queue, thread);
struct thread_queue threads;

static void free_thread(struct thread *thread) {
	debug("%hd is being freed…", thread->id)

	if (thread->valgrind_stack != -1)
		VALGRIND_STACK_DEREGISTER(thread->valgrind_stack);

	free(thread->context.uc_stack.ss_sp);
	free(thread);
}

__attribute__((unused)) __attribute__((constructor))
static void initialize_threads() {
	TAILQ_INIT(&threads);

	// Create the main thread (so it can call thread_self and thread_yield)
	struct thread *main_thread = malloc(sizeof *main_thread);
	main_thread->return_value = NULL;
	main_thread->context.uc_stack.ss_sp = NULL;
	main_thread->is_zombie = 1;
#ifdef USE_DEBUG
	main_thread->id = next_thread_id++;
#endif
	main_thread->valgrind_stack = -1;

	TAILQ_INSERT_HEAD(&threads, main_thread, entries);
	debug("%hd is the main thread.",
	      main_thread->id)
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
}

//endregion

static struct thread *thread_self_safe(void) {
	return TAILQ_FIRST(&threads);
}

thread_t thread_self(void) {
	return thread_self_safe();
}

void func_and_exit(void *(*func)(void *), void *func_arg) {
	thread_exit(func(func_arg));
}

int thread_create(thread_t *new_thread, void *(*func)(void *), void *func_arg) {
	struct thread *new = malloc(sizeof *new);
	if (new == NULL) {
		error("New thread allocation %s", "failed")
		exit(1);
	}

	if (getcontext(&new->context) == -1) {
		error("Failed to get context: %hd", new->id)
		exit(1);
	}

	new->context.uc_stack.ss_size = STACK_SIZE;
	new->context.uc_stack.ss_sp = malloc(STACK_SIZE);
	if (new->context.uc_stack.ss_sp == NULL) {
		error("New thread stack allocation failed: %hd",
		      new->id);
		exit(1);
	}

	new->context.uc_link = NULL;
	new->is_zombie = 1;
	makecontext(&new->context, (void (*)(void)) func_and_exit, 2, func, func_arg);

	new->return_value = NULL;
#ifdef USE_DEBUG
	new->id = next_thread_id++;
#endif
	new->valgrind_stack = VALGRIND_STACK_REGISTER(new->context.uc_stack.ss_sp,
	                                              new->context.uc_stack.ss_sp +
	                                              new->context.uc_stack.ss_size);
	*new_thread = new;
	info("%hd was just created.", new->id)

	TAILQ_INSERT_TAIL(&threads, new, entries);

	return thread_yield();
}

static int thread_is_alone(void) {
	assert(!TAILQ_EMPTY(&threads));
	return TAILQ_FIRST(&threads) == TAILQ_LAST(&threads, thread_queue);
}

int thread_yield(void) {
	if (thread_is_alone()) {
		debug("%hd: No thread to yield to, noop.", thread_self_safe()->id)
		// No thread to yield to: there is only one thread
		return 0;
	} else {
		struct thread *current = TAILQ_FIRST(&threads);
		assert(current);

		TAILQ_REMOVE(&threads, current, entries);
		TAILQ_INSERT_TAIL(&threads, current, entries);

		struct thread *next = TAILQ_FIRST(&threads);
		assert(next);

		debug("yield: %hd -> %hd", current->id, next->id)
		return swapcontext(&current->context, &next->context);
	}
}

int thread_join(thread_t thread, void **return_value) {
	struct thread *target = thread;
	info("%hd: Will join %hd", thread_self_safe()->id, target->id)

	while (1) {
		// we don't loop in zombies now, just check if the target thread is a zombie (checking his is_zombie)
		if (target->is_zombie == 0) {
			debug("%hd: Found zombie %hd…", thread_self_safe()->id,
			      target->id)
			if (return_value != NULL) {
				*return_value = target->return_value;
			}
			free_thread(target);
			return 0;
		}

		if (thread_is_alone()) {
			error("%hd: Trying to join %hd, yet there is no thread to yield to. This should not happen.",
			      thread_self_safe()->id, target->id)
			exit(1);
		}
		thread_yield();
	}
}

void thread_exit(void *return_value) {
	struct thread *current = TAILQ_FIRST(&threads);
	assert(current);
	current->return_value = return_value;
	TAILQ_REMOVE(&threads, current, entries);
	current->is_zombie = 0;
	info("%hd has died with return value %p.", current->id, return_value)

	if (TAILQ_EMPTY(&threads)) {
		info("All threads are dead: %s", "now terminating")
		exit(EXIT_SUCCESS);
	}

	struct thread *next = TAILQ_FIRST(&threads);
	debug("The execution will now move to %hd.", next->id)
	setcontext(&next->context);
}
