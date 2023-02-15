#ifndef MATRIX_H
#define MATRIX_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define PARTITION_SIZE(a, b) (a/b)
#define RANGE_START(a, b) a * b
#define RANGE_END(a, b) a * (b + 1)

/* Structs */
/* Matrix struct */
struct matrix_t {
    int **values;
    int cols;
    int rows;
};

struct shared_data_t {
    struct matrix_t *m_1;
    struct matrix_t *m_2;
    struct matrix_t *ans;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int n_threads;
};

struct thread_data_t {
    int row;
    struct shared_data_t *shared;
};

/* Methods */

/* Method to print a matrix. */
void print_matrix(struct matrix_t *matrix);

/* Method to allocate space for a matrix. */
struct matrix_t *malloc_matrix(int rows, int cols, int default_val);

/* Destructor method for matrixes. */
void free_matrix(struct matrix_t *matrix);

/* Matrix multiplication for multiple threads. */
/* Returns NULL if the arguments are illegal. */
struct matrix_t *matrix_mult_threaded(struct matrix_t *m_1, struct matrix_t *m_2);

/* Matrix multiplication for single threads. */
/* Returns NULL if the arguments are illegal. */
struct matrix_t *matrix_mult_single(struct matrix_t *m_1, struct matrix_t *m_2);

#endif