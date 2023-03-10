#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include "queue.h"
#include "worker.h"

struct worker_threads_t *malloc_worker_threads(int max_threads, int queue_size)
{
    struct worker_threads_t *res;
    struct worker_thread_t *worker;

    res = (struct worker_threads_t *)(malloc(sizeof(struct worker_threads_t)));
    
    res->task_queue = malloc_queue(queue_size);

    res->max_threads = max_threads;
    res->threads = (pthread_t *)(malloc(max_threads * sizeof(pthread_t)));
    
    res->cond_var = (struct condition_t *)(malloc(sizeof(struct condition_t)));
    res->cond_var->cond_predicate = true;
    pthread_mutex_init(&res->cond_var->cond_lock, NULL);
    pthread_cond_init(&res->cond_var->cond_variable, NULL);

    return res;
}

void worker_threads_stop(struct worker_threads_t *workers)
{
    pthread_mutex_lock(&workers->cond_var->cond_lock);
    workers->cond_var->cond_predicate = false;
    pthread_cond_broadcast(&workers->cond_var->cond_variable);
    pthread_mutex_unlock(&workers->cond_var->cond_lock);
    for (int i = 0; i < workers->max_threads; i++)
        pthread_join(*(workers->threads + i), NULL);
}

void free_worker_threads(struct worker_threads_t *workers)
{
    free_queue(workers->task_queue);
    free(workers->threads);
    pthread_mutex_destroy(&workers->cond_var->cond_lock);
    pthread_cond_destroy(&workers->cond_var->cond_variable);
    free(workers->cond_var);
    free(workers);
}

static void *start_worker_thread(void *arg)
{
    struct worker_threads_t *data = (struct worker_threads_t *)arg;
    while(true) {
        struct task_t *item;
        pthread_mutex_lock(&data->cond_var->cond_lock);
        while (queue_empty(data->task_queue) && data->cond_var->cond_predicate)
            pthread_cond_wait(&data->cond_var->cond_variable, &data->cond_var->cond_lock);
        item = (struct task_t *)queue_pop(data->task_queue);
        pthread_cond_signal(&data->cond_var->cond_variable);
        pthread_mutex_unlock(&data->cond_var->cond_lock);
        
        if (item != NULL) {
            usleep(item->sleep_time);
            item->func(item->arg);
            free(item);
        } else {
            pthread_exit(NULL);
        }
    }
}

void start_worker_threads(struct worker_threads_t *workers)
{
    for (int i = 0; i < workers->max_threads; i++)
        pthread_create(workers->threads + i, NULL, start_worker_thread, workers);
}

static bool submit_task(struct worker_threads_t *workers, struct task_t *task)
{
    pthread_mutex_lock(&workers->cond_var->cond_lock);
    bool ret = queue_push(workers->task_queue, (void *)task);
    while (!ret) {
        pthread_cond_wait(&workers->cond_var->cond_variable, &workers->cond_var->cond_lock);
        ret = queue_push(workers->task_queue, (void *)task);
    }
    pthread_mutex_unlock(&workers->cond_var->cond_lock);
    pthread_cond_signal(&workers->cond_var->cond_variable);
    return ret;
}

bool submit_worker_task(struct worker_threads_t *workers, worker_thread_func func, void *arg)
{
    struct task_t *task = (struct task_t*)(malloc(sizeof(struct task_t)));
    task->func = func;
    task->arg = arg;
    task->sleep_time = 0;
    return submit_task(workers, task);
}

bool submit_worker_task_timeout(struct worker_threads_t *workers, worker_thread_func func, void *arg, int timeout)
{
    struct task_t *task = (struct task_t*)(malloc(sizeof(struct task_t)));
    task->func = func;
    task->arg = arg;
    task->sleep_time = timeout;
    return submit_task(workers, task);
}