#ifndef WORKER_H
#define WORKER_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include "queue.h"

/* Standard function for a worker thread. */
typedef void worker_thread_func(void *arg);

/* Structs */

/* Task */
struct task_t {
    worker_thread_func *func;
    void *arg;
    int sleep_time;
};

/* Condition */
struct condition_t {
    bool cond_predicate;
    pthread_mutex_t cond_lock;
    pthread_cond_t cond_variable;
};

/* Worker Threads */
struct worker_threads_t {
    int max_threads;
    struct queue_t *task_queue;
    struct condition_t *cond_var;
    pthread_t *threads;
};

/* Methods */

/* Method to allocate worker threads. */
struct worker_threads_t *malloc_worker_threads(int max_threads, int queue_size);

/* Method to free worker threads. */
void free_worker_threads(struct worker_threads_t *workers);

/* Method to start worker threads. */
void start_worker_threads(struct worker_threads_t *workers);

/* Method to submit a task for the working threads. */
bool submit_worker_task(struct worker_threads_t *workers, worker_thread_func func, void *arg);

/* Method to submit a task in a set period of time. */
bool submit_worker_task_timeout(struct worker_threads_t *workers, worker_thread_func func, void *arg, int timeout);

/* Method to stop a set of worker threads. */
void worker_threads_stop(struct worker_threads_t *workers);


#endif