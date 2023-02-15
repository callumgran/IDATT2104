#include <stdio.h>
#include <stdlib.h>

#include "worker.h"

void sum(void *arg)
{
    int* nums = (int *)arg;
    int a = *nums;
    int b = *(nums + 1);

    printf("%d + %d = %d\n", a, b, a + b);
    free(arg);
}

void times(void *arg)
{
    int* nums = (int *)arg;
    int a = *nums;
    int b = *(nums + 1);

    printf("%d * %d = %d\n", a, b, a * b);
    free(arg);
}

void task_a(void *arg)
{
    printf("TASK A.\n");
}

void task_b(void *arg)
{
    printf("TASK B.\n");
}

int main()
{
    struct worker_threads_t *workers = malloc_worker_threads(16, 16);
    struct worker_threads_t *event_loop = malloc_worker_threads(1, 2);

    start_worker_threads(workers);
    start_worker_threads(event_loop);

    for (int i = 0; i < 16; i++) {
        int a = rand() % 100;
        int b = rand() % 100;
        int *arg = (void *)(malloc(2 * sizeof(int)));
        *arg = a;
        *(arg + 1) = b;
        if (!submit_worker_task(workers, sum, (void *)arg)) {
            fprintf(stderr, "An error occured whilst submitting tasks.\n");
            exit(1);
        }
    }

    for (int i = 0; i < 16; i++) {
        int a = rand() % 100;
        int b = rand() % 100;
        int *arg = (void *)(malloc(2 * sizeof(int)));
        *arg = a;
        *(arg + 1) = b;
        if (!submit_worker_task(workers, times, (void *)arg)) {
            fprintf(stderr, "An error occured whilst submitting tasks.\n");
            exit(1);
        }
    }

    submit_worker_task_timeout(event_loop, task_a, NULL, 1000);

    submit_worker_task_timeout(event_loop, task_b, NULL, 200);

    worker_threads_stop(workers);
    worker_threads_stop(event_loop);

    free_worker_threads(workers);
    free_worker_threads(event_loop);
    
    return 0;
}