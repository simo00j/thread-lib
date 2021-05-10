#include <stdio.h>
#include <ucontext.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <valgrind/valgrind.h>
#include <signal.h>
#include <limits.h>
#include <assert.h>
#include "thread.h"
#include "debug.h"

#define STACK_SIZE (64 * 1024)

#ifdef USE_DEBUG
static short next_thread_id = 0;
#endif

//region Structure declaration

struct sig {
	char received; // 1 = yes, 0 = non
	sighandler_t handler;
};

struct thread {
	ucontext_t context;
	void *return_value;
#ifdef USE_DEBUG
	short id;
#endif
	unsigned int valgrind_stack;
	STAILQ_ENTRY(thread) entries;
	/*
	 * Array of signals handler
	 */
	struct sig *sig_handler_table;

	/**
	 * Is this thread a zombie? (= called exit, but hasn't been joined yet)
	 * 1 = zombie, 0 = active
	 */
	char is_zombie;

	/**
	 * The thread responsible for joining this one.
	 */
	struct thread *joiner;
};

STAILQ_HEAD(thread_queue, thread);
struct thread_queue threads;
static struct thread *thread_self_safe(void);
struct thread *main_thread, *current_to_free = NULL;


void signal_create(struct thread *thread){
	thread->sig_handler_table = calloc(_POSIX_SIGQUEUE_MAX, sizeof(struct sig));
	if (thread->sig_handler_table == NULL) {
		error("New signal table allocation failed: %hd", thread->id);
		exit(1);
	}
	for(int i = 0; i < _POSIX_SIGQUEUE_MAX; i++){
		thread->sig_handler_table[i].handler = SIG_DFL;
	}
}

void free_signal(struct thread *thread){
	free(thread->sig_handler_table);
}

void check_signals(struct thread *thread){
	for(int i = 0; i < _POSIX_SIGQUEUE_MAX; i++){
		if(thread->sig_handler_table[i].received){
			thread->sig_handler_table[i].received = 0;
			ucontext_t *signal_context = malloc(sizeof(ucontext_t));
			signal_context->uc_stack.ss_size = STACK_SIZE;
			signal_context->uc_stack.ss_sp = malloc(STACK_SIZE);
			if (signal_context->uc_stack.ss_sp == NULL) {
				error("New thread stack allocation failed: %hd", new->id);
				exit(1);
			}
			signal_context->uc_link = &thread_self_safe()->context;
			makecontext(signal_context, thread->sig_handler_table[i].handler,0);
			swapcontext(&thread_self_safe()->context, signal_context);
			free(signal_context->uc_stack.ss_sp);
			free(signal_context);
		}
	}
}

sighandler_t thread_signal(int signum, sighandler_t handler){
	struct thread *current = STAILQ_FIRST(&threads);
	assert(current);

	sighandler_t old_handler = current->sig_handler_table[signum].handler;
	current->sig_handler_table[signum].handler = handler;
	return old_handler;
}

int thread_kill(thread_t thread, int signum){
	if(signum < 1 || signum > _POSIX_SIGQUEUE_MAX){
		return -1;
	}
	struct thread *target = thread;
	target->sig_handler_table[signum].received = 1;
	return 0;
}


static void free_thread(struct thread *thread) {
	debug("%hd is being freed, on address %p", thread->id, (void *) thread)

	if (thread->valgrind_stack != -1)
		VALGRIND_STACK_DEREGISTER(thread->valgrind_stack);

	free_signal(thread);
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
	main_thread->joiner = NULL;
#ifdef USE_DEBUG
	main_thread->id = next_thread_id++;
#endif
	signal_create(main_thread);
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
	new->joiner = NULL;
	makecontext(&new->context, (void (*)(void)) func_and_exit, 2, func, func_arg);

	new->return_value = NULL;
#ifdef USE_DEBUG
	new->id = next_thread_id++;
#endif

	signal_create(new);

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
		int swap_ret_value = swapcontext(&current->context, &next->context);
		check_signals(thread_self_safe());
		return swap_ret_value;
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

	if (target->joiner != NULL) {
		error("%hd: The thread %hd has already been claimed for join by the thread %hd",
		      thread_self_safe()->id, target->id, target->joiner->id)
		return -1;
	}
	target->joiner = thread_self_safe();

	if (!target->is_zombie) { // the target hasn't died yet
		struct thread *current = thread_self_safe();
		STAILQ_REMOVE_HEAD(&threads, entries); // I'm not alive anymore

		if (STAILQ_EMPTY(&threads)) {
			error("%hd: I'm the last thread alive, but I was asked to join %hd, which is not dead. This is impossible.",
			      current->id, target->id)
			STAILQ_INSERT_HEAD(&threads, current, entries);
			target->joiner = NULL;
			return -1;
		}

		// Yield to another thread, the one I'm waiting for will add me back to the live threads
		thread_yield_from(current);
	}
	assert(target->is_zombie);

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

void thread_exit(void *return_value) {
	struct thread *current = STAILQ_FIRST(&threads);
	assert(current);

	current->return_value = return_value;
	STAILQ_REMOVE_HEAD(&threads, entries);
	current->is_zombie = 1;

	if (current->joiner != NULL)
		STAILQ_INSERT_TAIL(&threads, current->joiner, entries);

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
