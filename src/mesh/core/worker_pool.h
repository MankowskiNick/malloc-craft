#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <pthread.h>

typedef struct worker_pool worker_pool;

// Callback function type for processing work items
typedef void (*work_processor_fn)(void* work_item);

/**
 * Initialize a worker pool with a specified number of threads
 * @param num_threads Number of worker threads to create (1-16 recommended)
 * @param process_fn Callback function to process each work item
 * @return Pointer to allocated worker pool, or NULL on failure
 */
worker_pool* pool_init(int num_threads, work_processor_fn process_fn);

/**
 * Submit work to the worker pool
 * @param pool Pointer to worker pool
 * @param work_item Pointer to work item to process
 * @return 0 on success, -1 on failure
 */
int pool_submit_work(worker_pool* pool, void* work_item);

/**
 * Check if work pool has any pending work
 * @param pool Pointer to worker pool
 * @return 1 if idle (no pending work), 0 if busy
 */
int pool_is_idle(worker_pool* pool);

/**
 * Wait until all submitted work is complete
 * @param pool Pointer to worker pool
 */
void pool_wait_completion(worker_pool* pool);

/**
 * Shutdown the worker pool and clean up all threads
 * @param pool Pointer to worker pool
 */
void pool_shutdown(worker_pool* pool);

#endif
