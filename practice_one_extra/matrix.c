#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "matrix.h"

/* Predicate function for checking multiplicity of matrices. */
static inline bool check_mult(struct matrix_t *m_1, struct matrix_t *m_2)
{
    return m_1->cols != m_2->rows;
}

static void *mult_thread(void *arg)
{
    struct thread_data_t *tdata = (struct thread_data_t *)arg;
    int** m_1_values = tdata->shared->m_1->values;
    int** m_2_values = tdata->shared->m_2->values;
    int** ans_values = tdata->shared->ans->values;

    for (int j = 0; j < tdata->shared->m_2->cols; j++)
        for (int k = 0; k < tdata->shared->m_1->cols; k++)
            *(*(ans_values + tdata->row) + j) += *(*(m_1_values + tdata->row) + k) * *(*(m_2_values + k) + j);    

    free(tdata);
}

static struct shared_data_t *malloc_shared_data(struct matrix_t *m_1, struct matrix_t *m_2, struct matrix_t *ans)
{
    struct shared_data_t *shared = (struct shared_data_t *)(malloc(sizeof(struct shared_data_t)));

    shared->m_1 = m_1;
    shared->m_2 = m_2;
    shared->ans = ans;
    shared->n_threads = 0;
    pthread_mutex_init(&shared->mutex, NULL);
    pthread_cond_init(&shared->cond, NULL);

    return shared;
}

struct matrix_t *malloc_matrix(int rows, int cols, int default_val)
{
    struct matrix_t *res = (struct matrix_t *)(malloc(sizeof(struct matrix_t)));

    res->values = (int **)(calloc(rows, sizeof(int *)));
    for (int i = 0; i < rows; i++) {
        *(res->values + i) = (int *)(calloc(cols, sizeof(int)));
        for (int j = 0; j < cols; j++)
            *(*(res->values + i) + j) = default_val;
    }

    res->rows = rows;
    res->cols = cols;

    return res;
}

void free_matrix(struct matrix_t *matrix)
{
    for (int i = 0; i < matrix->rows; i++)
        free(*(matrix->values + i));
    free(matrix->values);
    free(matrix);
}

static inline struct matrix_t *gen_ans(struct matrix_t *m_1, struct matrix_t *m_2)
{
    if (check_mult(m_1, m_2))
        return NULL;

    return malloc_matrix(m_1->rows, m_2->cols, 0);
}

struct matrix_t *matrix_mult_threaded(struct matrix_t *m_1, struct matrix_t *m_2)
{
    pthread_t threads[m_1->rows];
    struct matrix_t *ans = gen_ans(m_1, m_2);

    if (!ans)
        return NULL;

    struct shared_data_t *shared = malloc_shared_data(m_1, m_2, ans);

    for (int i = 0; i < m_1->rows; i++) {
        struct thread_data_t *tdata = (struct thread_data_t *)(malloc(sizeof(struct thread_data_t)));

        tdata->shared = shared;
        tdata->row = i;

        pthread_create(threads + i, NULL, mult_thread, (void *)tdata);
    }

    for (int i = 0; i < m_1->rows; i++)
        pthread_join(*(threads + i), NULL);

    free(shared);

    return ans;
}

struct matrix_t *matrix_mult_single(struct matrix_t *m_1, struct matrix_t *m_2)
{
    struct matrix_t *ans = gen_ans(m_1, m_2);

    if (!ans)
        return NULL;
    
    for (int i = 0; i < m_1->rows; i++)
        for (int j = 0; j < m_2->cols; j++)
            for (int k = 0; k < m_1->cols; k++) 
                *(*(ans->values + i) + j) += *(*(m_1->values + i) + k) * *(*(m_2->values + k) + j);

    return ans;
}

void print_matrix(struct matrix_t *matrix)
{
    for (int i = 0; i < matrix->rows; i++) {
        for (int j = 0; j < matrix->cols; j++)
            printf("[%d],", *(*(matrix->values + i ) + j));
        putchar('\n');
    }
}
