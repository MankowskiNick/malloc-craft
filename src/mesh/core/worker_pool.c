#include "worker_pool.h"
#include "../../util/queue.h"
#include "../generation/chunk_mesh.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    pthread_t thread_id;
    int worker_id;
    int is_active;
    void* pool_ref;  // Back-reference to pool
} worker_thread;

struct worker_pool {
    worker_thread* threads;
    int num_threads;
    
    queue_node* work_queue;
    work_processor_fn process_fn;  // Function to process work items
    
    pthread_mutex_t work_queue_mutex;
    pthread_cond_t work_available_signal;
    
    int shutdown_flag;
    int pending_work_count;
    pthread_mutex_t pending_work_mutex;
    pthread_cond_t all_work_done_signal;
};

/**
 * Worker thread main loop
 */
void* worker_thread_main(void* arg) {
    worker_thread* self = (worker_thread*)arg;
    worker_pool* pool = (worker_pool*)self->pool_ref;
    
    while (!pool->shutdown_flag) {
        pthread_mutex_lock(&pool->work_queue_mutex);
        
        // Wait for work or shutdown signal
        while (pool->work_queue == NULL && !pool->shutdown_flag) {
            pthread_cond_wait(&pool->work_available_signal, &pool->work_queue_mutex);
        }
        
        if (pool->shutdown_flag) {
            pthread_mutex_unlock(&pool->work_queue_mutex);
            break;
        }
        
        // Pop work from queue
        void* work_item = queue_pop(&pool->work_queue);
        pthread_mutex_unlock(&pool->work_queue_mutex);
        
        if (work_item == NULL) {
            continue;
        }
        
        // Process work item using the processor function
        if (pool->process_fn != NULL) {
            pool->process_fn(work_item);
        }
        
        // Decrement pending work counter
        pthread_mutex_lock(&pool->pending_work_mutex);
        pool->pending_work_count--;
        if (pool->pending_work_count == 0) {
            pthread_cond_signal(&pool->all_work_done_signal);
        }
        pthread_mutex_unlock(&pool->pending_work_mutex);
    }
    
    self->is_active = 0;
    return NULL;
}

worker_pool* pool_init(int num_threads, work_processor_fn process_fn) {
    if (num_threads < 1 || num_threads > 16) {
        return NULL;
    }
    
    if (process_fn == NULL) {
        return NULL;
    }
    
    worker_pool* pool = (worker_pool*)malloc(sizeof(worker_pool));
    if (pool == NULL) {
        return NULL;
    }
    
    pool->num_threads = num_threads;
    pool->process_fn = process_fn;
    pool->threads = (worker_thread*)malloc(sizeof(worker_thread) * num_threads);
    if (pool->threads == NULL) {
        free(pool);
        return NULL;
    }
    
    pool->work_queue = NULL;
    pool->shutdown_flag = 0;
    pool->pending_work_count = 0;
    
    queue_init(&pool->work_queue);
    
    pthread_mutex_init(&pool->work_queue_mutex, NULL);
    pthread_mutex_init(&pool->pending_work_mutex, NULL);
    pthread_cond_init(&pool->work_available_signal, NULL);
    pthread_cond_init(&pool->all_work_done_signal, NULL);
    
    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        pool->threads[i].worker_id = i;
        pool->threads[i].is_active = 1;
        pool->threads[i].pool_ref = pool;
        
        if (pthread_create(&pool->threads[i].thread_id, NULL, worker_thread_main, &pool->threads[i]) != 0) {
            // Failed to create thread, cleanup
            pool->shutdown_flag = 1;
            pthread_cond_broadcast(&pool->work_available_signal);
            
            for (int j = 0; j < i; j++) {
                pthread_join(pool->threads[j].thread_id, NULL);
            }
            
            pthread_mutex_destroy(&pool->work_queue_mutex);
            pthread_mutex_destroy(&pool->pending_work_mutex);
            pthread_cond_destroy(&pool->work_available_signal);
            pthread_cond_destroy(&pool->all_work_done_signal);
            
            queue_cleanup(&pool->work_queue);
            
            free(pool->threads);
            free(pool);
            return NULL;
        }
    }
    
    return pool;
}

int pool_submit_work(worker_pool* pool, void* work_item) {
    if (pool == NULL || work_item == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->work_queue_mutex);
    queue_push(&pool->work_queue, work_item, chunk_work_item_equals);
    
    pthread_mutex_lock(&pool->pending_work_mutex);
    pool->pending_work_count++;
    pthread_mutex_unlock(&pool->pending_work_mutex);
    
    pthread_cond_signal(&pool->work_available_signal);
    pthread_mutex_unlock(&pool->work_queue_mutex);
    
    return 0;
}

int pool_is_idle(worker_pool* pool) {
    if (pool == NULL) {
        return 1;
    }
    
    pthread_mutex_lock(&pool->pending_work_mutex);
    int idle = (pool->pending_work_count == 0);
    pthread_mutex_unlock(&pool->pending_work_mutex);
    
    return idle;
}

void pool_wait_completion(worker_pool* pool) {
    if (pool == NULL) {
        return;
    }
    
    pthread_mutex_lock(&pool->pending_work_mutex);
    while (pool->pending_work_count > 0) {
        pthread_cond_wait(&pool->all_work_done_signal, &pool->pending_work_mutex);
    }
    pthread_mutex_unlock(&pool->pending_work_mutex);
}

void pool_shutdown(worker_pool* pool) {
    if (pool == NULL) {
        return;
    }
    
    // Signal all threads to shutdown
    pthread_mutex_lock(&pool->work_queue_mutex);
    pool->shutdown_flag = 1;
    pthread_cond_broadcast(&pool->work_available_signal);
    pthread_mutex_unlock(&pool->work_queue_mutex);
    
    // Wait for all threads to finish
    for (int i = 0; i < pool->num_threads; i++) {
        if (pool->threads[i].is_active) {
            pthread_join(pool->threads[i].thread_id, NULL);
        }
    }
    
    // Clean up resources
    pthread_mutex_destroy(&pool->work_queue_mutex);
    pthread_mutex_destroy(&pool->pending_work_mutex);
    pthread_cond_destroy(&pool->work_available_signal);
    pthread_cond_destroy(&pool->all_work_done_signal);
    
    queue_cleanup(&pool->work_queue);
    
    free(pool->threads);
    free(pool);
}
