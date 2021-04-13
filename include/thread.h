#ifndef OS_S8_THREAD_H
#define OS_S8_THREAD_H

#ifndef USE_PTHREAD

/**
 * Thread identifier.
 */
// Could be changed to an integer instead of a pointer,
// but thread arrays would be harder (memory consumption, allocation cost).
typedef void *thread_t;

/**
 * Get the identifier of the current thread.
 * @return The identifier of the current thread.
 */
extern thread_t thread_self(void);

/**
 * Create a new thread.
 * @param new_thread The identifier of the new thread (allocate the pointer, the function will return it)
 * @param func The function executed by the new thread
 * @param func_arg Arguments passed to the function func
 * @return 0 on success, -1 on failure
 */
extern int thread_create(thread_t *new_thread, void *(*func)(void *), void *func_arg);

/**
 * Let another thread take control.
 * @return 0 on success, -1 on failure
 */
extern int thread_yield(void);

/**
 * Wait for a thread to terminate.
 *
 * If two threads try to join the same one, the results are undefined.
 * @param thread The thread we're waiting for
 * @param return_value The thread's return value is placed here. If `NULL` is passed, the return value is ignored.
 * @return 0 on success, an error number on failure.
 */
extern int thread_join(thread_t thread, void **return_value);

/**
 * Terminate the calling thread and returns a value.
 *
 * This function never returns.
 * @param return_value The return value captured by thread_join
 */
extern void thread_exit(void *return_value); //TODO: ajouter "__attribute__ ((__noreturn__))" quand cette fonction sera implémentée

/* Interface possible pour les mutex */
typedef struct thread_mutex { int dummy; } thread_mutex_t;

int thread_mutex_init(thread_mutex_t *mutex);

int thread_mutex_destroy(thread_mutex_t *mutex);

int thread_mutex_lock(thread_mutex_t *mutex);

int thread_mutex_unlock(thread_mutex_t *mutex);

#else /* USE_PTHREAD */

/* Si on compile avec -DUSE_PTHREAD, ce sont les pthreads qui sont utilisés */
#include <sched.h>
#include <pthread.h>
#define thread_t pthread_t
#define thread_self pthread_self
#define thread_create(th, func, arg) pthread_create(th, NULL, func, arg)
#define thread_yield sched_yield
#define thread_join pthread_join
#define thread_exit pthread_exit

/* Interface possible pour les mutex */
#define thread_mutex_t            pthread_mutex_t
#define thread_mutex_init(_mutex) pthread_mutex_init(_mutex, NULL)
#define thread_mutex_destroy      pthread_mutex_destroy
#define thread_mutex_lock         pthread_mutex_lock
#define thread_mutex_unlock       pthread_mutex_unlock

#endif /* USE_PTHREAD */

#endif //OS_S8_THREAD_H
