#include <stdio.h>
#include <ucontext.h>
#include <sys/queue.h>
#include <stdlib.h>
#include "thread.h"
#include <valgrind/valgrind.h>
#include <errno.h>
#include "debug.h"
#include <assert.h>

#define STACK_SIZE (64 * 1024)

#ifdef USE_DEBUG

#include <valgrind/valgrind.h>

static short next_thread_id = 0;
#endif

//region Structure declaration

struct thread {
	ucontext_t context;
	void *return_value;
	void *stack;
#ifdef USE_DEBUG
	short id;
#endif
	unsigned int valgrind_stack;
	char is_zombie; // 1 = zombie, 0 = active
	STAILQ_ENTRY(thread) entries;
};

STAILQ_HEAD(thread_queue, thread);
struct thread_queue threads;

struct thread *main_thread, *current_to_free = NULL;

static void free_thread(struct thread *thread) {
	debug("%hd is being freed, on address %p", thread->id, (void *) thread)

	if (thread->valgrind_stack != -1)
		VALGRIND_STACK_DEREGISTER(thread->valgrind_stack);

	free(thread->context.uc_stack.ss_sp);
	free(thread);
}

__attribute__((unused)) __attribute__((constructor))
static void initialize_threads() {
	STAILQ_INIT(&threads);

	// Create the main thread (so it can call thread_self and thread_yield)
	main_thread = malloc(sizeof *main_thread);
	main_thread->return_value = NULL;
	main_thread->context.uc_stack.ss_sp = NULL;
	main_thread->is_zombie = 0;
#ifdef USE_DEBUG
	main_thread->id = next_thread_id++;
#endif
	main_thread->valgrind_stack = -1;

	STAILQ_INSERT_HEAD(&threads, main_thread, entries);
	debug("%hd is the main thread.", main_thread->id)
}

__attribute__((unused)) __attribute__((destructor))
static void free_threads() {
	printf("\n");
	info("%s, now freeing all remaining threads…", "Program has exited")

	struct thread *current = STAILQ_FIRST(&threads);
	struct thread *next;

	while (current != NULL) {
		next = STAILQ_NEXT(current, entries);
		if (current != main_thread)
			free_thread(current);
		current = next;
	}

	free_thread(main_thread);

	if (current_to_free != NULL)
		free_thread(current_to_free);
}

//endregion

static struct thread *thread_self_safe(void) {
	return STAILQ_FIRST(&threads);
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
		error("New thread stack allocation failed: %hd", new->id);
		exit(1);
	}

	new->context.uc_link = &main_thread->context;
	new->is_zombie = 0;
	makecontext(&new->context, (void (*)(void)) func_and_exit, 2, func, func_arg);

	new->return_value = NULL;
#ifdef USE_DEBUG
	new->id = next_thread_id++;
#endif
	new->valgrind_stack = VALGRIND_STACK_REGISTER(new->context.uc_stack.ss_sp,
	                                              new->context.uc_stack.ss_sp +
	                                              new->context.uc_stack.ss_size);
	*new_thread = new;
	info("%hd was just created, on address %p", new->id, (void *) new)

	STAILQ_INSERT_TAIL(&threads, new, entries);

	return thread_yield();
}

static int thread_is_alone(void) {
	assert(!STAILQ_EMPTY(&threads));
	return STAILQ_NEXT(STAILQ_FIRST(&threads), entries) == NULL;
}

/**
 * Yield to another thread, from current.
 * @param current The current thread. Should be at the end of the queue.
 */
static int thread_yield_from(struct thread *current) {
	assert(current);

	struct thread *next = STAILQ_FIRST(&threads);
	assert(next);

	if (next == current) {
		debug("%hd: No thread to yield to, noop.", thread_self_safe()->id)
		return 0;
	} else {
		debug("yield: %hd -> %hd", current->id, next->id)
		return swapcontext(&current->context, &next->context);
	}
}

int thread_yield(void) {
	if (thread_is_alone()) {
		debug("%hd: No thread to yield to, noop.", thread_self_safe()->id)
		// No thread to yield to: there is only one thread
		return 0;
	} else {
		struct thread *current = STAILQ_FIRST(&threads);
		assert(current);

		STAILQ_REMOVE_HEAD(&threads, entries);
		STAILQ_INSERT_TAIL(&threads, current, entries);

		return thread_yield_from(current);
	}
}

int thread_join(thread_t thread, void **return_value) {
	struct thread *target = thread;
	info("%hd: Will join %hd", thread_self_safe()->id, target->id)

	while (1) {
		if (target->is_zombie) {
			debug("%hd: will free %hd", thread_self_safe()->id, target->id)
			if (return_value != NULL) {
				*return_value = target->return_value;
			}

			if (target != main_thread)
				free_thread(target);
			else
				debug("Detected and cancelled an attempt to free %s.", "the main thread")

			return 0;
		}

		if (thread_is_alone()) {
			error("%hd: Trying to join %hd, yet there is no thread to yield to. This should not happen.",
			      thread_self_safe()->id, target->id)
			return -1;
		}
		thread_yield();
	}
}

void thread_exit(void *return_value) {
	struct thread *current = STAILQ_FIRST(&threads);
	assert(current);
	current->return_value = return_value;
	STAILQ_REMOVE_HEAD(&threads, entries);
	current->is_zombie = 1;
	info("%hd has died with return value %p.", current->id, return_value)

	if (STAILQ_EMPTY(&threads)) {
		info("All threads are dead: %s", "forcing termination")
		current_to_free = current;
		setcontext(&main_thread->context);
	} else {
		struct thread *next = STAILQ_FIRST(&threads);
		debug("The execution will now move to %hd.", next->id)
		swapcontext(&current->context, &next->context);
	}
}

//region Mutex

int thread_mutex_init(thread_mutex_t *mutex) {
	mutex->owner = NULL;
	STAILQ_INIT(&mutex->waiting_queue);
	debug("Created mutex %p", (void *) mutex)
	return 0;
}

int thread_mutex_destroy(thread_mutex_t *mutex) {
	debug("Destroying mutex %p", (void *) mutex)
	if (!STAILQ_EMPTY(&mutex->waiting_queue)) {
		warn("Attempted to destroy a owner mutex: %p", (void *) mutex)
		thread_mutex_unlock(mutex);
		perror("Ebusy"); //FIXME: faire planter
	}
	return 0;
}

int thread_mutex_lock(thread_mutex_t *mutex) {
	do {
		if (mutex->owner == NULL) {
			debug("%d: Locking mutex %p", thread_self_safe()->id, (void *) mutex)
			mutex->owner = thread_self();
		} else if (mutex->owner == thread_self_safe()) {
			// Nothing to do, I'm already the owner
		} else {
			struct thread *current = thread_self_safe();
			debug("%d: Mutex %p is already owner", current->id, (void *) mutex)
			STAILQ_REMOVE(&threads, current, thread, entries);
			STAILQ_INSERT_TAIL(&mutex->waiting_queue,
			                   current,
			                   entries);
			thread_yield_from(current);
		}
	} while (mutex->owner != thread_self());
	return 0;
}

int thread_mutex_unlock(thread_mutex_t *mutex) {
	debug("%d: Unlocking mutex %p", thread_self_safe()->id, (void *) mutex)
	if (!STAILQ_EMPTY(&mutex->waiting_queue)) {
		struct thread *next_thread = STAILQ_FIRST(&mutex->waiting_queue);
		STAILQ_REMOVE_HEAD(&mutex->waiting_queue, entries);
		STAILQ_INSERT_TAIL(&threads, next_thread, entries);
		mutex->owner = next_thread;
	} else {
		mutex->owner = NULL;
	}
	return 0;
}

//endregion
