#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#define PARTITION_SIZE(a, b) (a/b)
#define RANGE_START(a, b) a * b
#define RANGE_END(a, b) a * (b + 1)
#define START_CAP 32

struct list_t {
    int size;
    int cap;
    int *nums;
};

struct shared_params_t {
    int max_threads;
    int n_threads;
    int start;
    int end;
    bool *prime;
    struct list_t *help_list;
    pthread_cond_t t_cond;
    pthread_mutex_t t_mutex;
};

struct tparams_t {
    struct shared_params_t *shared;
    int start;
    int end;
};

struct list_t *list_malloc(int cap)
{
    struct list_t *list = (struct list_t *)(malloc)(sizeof(struct list_t));
    list->size = 0;
    list->cap = cap;
    list->nums = (int *)(malloc(list->cap * sizeof(int)));
    return list;
}

struct shared_params_t *shared_params_malloc(int threads, int start, int end)
{
    struct shared_params_t *s_params = (struct shared_params_t *)(malloc(sizeof(struct shared_params_t)));
    pthread_cond_init(&s_params->t_cond, NULL);
    pthread_mutex_init(&s_params->t_mutex, NULL);
    s_params->max_threads = threads;
    s_params->n_threads = 0;
    s_params->start = start;
    s_params->end = end;
    s_params->help_list = list_malloc(START_CAP);
    return s_params;
}

void ensure_capacity(struct list_t *list)
{
    if (list->size == list->cap) {
        list->cap = list->cap << 1;
        list->nums = (int *)(realloc(list->nums, list->cap * sizeof(int)));
    }
}

bool add_to_list(struct list_t *list, int num)
{
    ensure_capacity(list);
    *(list->nums + list->size++) = num;
    return true;
}

void fill_nums(struct list_t *list, int start, int end)
{
    bool *ck = (bool *)(malloc(sizeof(bool) * (end + 1)));
    memset(ck, true, sizeof(bool) * (end + 1));

    *ck = false;
    *(ck + 1) = false;

    int end_sqrt = sqrt(end);

    for (int i = 2; i <= end_sqrt; i++)
        if (*(ck + i)) 
            for (int j = i * i; j <= end_sqrt; j += i) 
                *(ck + j) = false;
        
    for (int i = 2; i * i <= end; i++)
        if (*(ck + i))
            add_to_list(list, i);

    free(ck);
}

void *sieve(void *arg)
{
    struct tparams_t *params = (struct tparams_t *) arg;

    for (int i = params->start; i < params->end; i++) {
        int lower = params->shared->start / *(params->shared->help_list->nums + i);

        if (lower <= 1)
            lower = *(params->shared->help_list->nums + i) + *(params->shared->help_list->nums + i);
        else if (params->shared->start % *(params->shared->help_list->nums + i))
            lower = (lower * *(params->shared->help_list->nums + i)) + *(params->shared->help_list->nums + i);
        else
            lower = (lower * *(params->shared->help_list->nums + i));
        
        for (int j = lower; j <= params->shared->end; j = j + *(params->shared->help_list->nums + i))
            *(params->shared->prime + j - params->shared->start) = false;
    }

    pthread_mutex_lock(&params->shared->t_mutex);
    if (--params->shared->n_threads == 0)
        pthread_cond_signal(&params->shared->t_cond);
    pthread_mutex_unlock(&params->shared->t_mutex);
    free(arg);
}

bool *find_primes(int start, int end, int max_threads)
{
    struct shared_params_t *shared = shared_params_malloc(max_threads, start, end);

    if (start < 2 && end >= 2)
        start = 2;

    fill_nums(shared->help_list, start, end);

    if (shared->help_list->size < max_threads) {
        max_threads = shared->help_list->size;
        shared->max_threads = max_threads;
    }

    bool *prime= (bool *)(malloc(sizeof(bool) * (end - start + 1)));
    memset(prime, true, sizeof(bool) * (end - start + 1));

    shared->prime = prime;

    pthread_t threads[max_threads];

    int partition = PARTITION_SIZE(shared->help_list->size, max_threads);

    int rests[max_threads];
    memset(rests, 0, max_threads * sizeof(int));

    int rest = shared->help_list->size % max_threads;

    if (rest != 0)
        for (int i = 0; i < rest; i++)
            (*(rests + i))++;

    for (int i = 0; i < max_threads; i++) {
        struct tparams_t *tparams = (struct tparams_t *)(malloc(sizeof(struct tparams_t)));

        tparams->shared = shared;

        shared->n_threads++;

        if (i == 0) {
            tparams->start = RANGE_START(partition, i);
            tparams->end = RANGE_END(partition, i) + *(rests + i);
        } else {
            if (*(rests + i)) {
                tparams->start = RANGE_START(partition, i) + i;
                tparams->end = RANGE_END(partition, i) + i;
            } else {
                tparams->start = RANGE_START(partition, i) + rest;
                tparams->end = RANGE_END(partition, i) + rest;
            }
        }

        pthread_create(threads + i, NULL, sieve, tparams);
    }

    pthread_mutex_lock(&shared->t_mutex);
    pthread_cond_wait(&shared->t_cond, &shared->t_mutex);
    pthread_mutex_unlock(&shared->t_mutex);

    free(shared->help_list->nums);
    free(shared->help_list);
    free(shared);

    return prime;
}

void print_list(bool *primes, int start, int end)
{
    for (int i = start; i <= end; i++)
        if (*(primes + i - start))
            printf("%d\n", i);
}

int main(int argc, char **argv)
{
    if (argc == 4) {
        int start = atoi(*(argv + 1));
        int end = atoi(*(argv + 2));
        int threads = atoi(*(argv + 3));
        
        struct timespec t_start, finish;

        clock_gettime(CLOCK_MONOTONIC_RAW, &t_start);
        bool *primes = find_primes(start, end, threads);
        clock_gettime(CLOCK_MONOTONIC_RAW, &finish);

        double elapsed = (finish.tv_sec - t_start.tv_sec);
        elapsed += (finish.tv_nsec - t_start.tv_nsec) / 1000000000.0;

        // print_list(primes, start, end);

        printf("Time used was %fms for the numbers between %d and %d. \n", elapsed * 1000, start, end);
        
        free(primes);
    } else {
        fprintf(stderr, "The input was formatted incorrectly.\n");
        exit(1);
    }
    return 0;
}
