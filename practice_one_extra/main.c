#include <stdlib.h>
#include <stdio.h>

#include "matrix.h"

int main(int argc, char** argv)
{
    struct timespec t_start, finish;
    
    struct matrix_t *m_1 = malloc_matrix(400, 400, 5);
    struct matrix_t *m_2 = malloc_matrix(400, 400, 2);

    clock_gettime(CLOCK_MONOTONIC_RAW, &t_start);
    struct matrix_t *ans_single = matrix_mult_single(m_1, m_2);
    clock_gettime(CLOCK_MONOTONIC_RAW, &finish);

    double elapsed = (finish.tv_sec - t_start.tv_sec);
    elapsed += (finish.tv_nsec - t_start.tv_nsec) / 1000000000.0;

    printf("Time used was %fms for single-threaded. \n", elapsed * 1000);

    clock_gettime(CLOCK_MONOTONIC_RAW, &t_start);
    struct matrix_t *ans_multi = matrix_mult_threaded(m_1, m_2);
    clock_gettime(CLOCK_MONOTONIC_RAW, &finish);

    elapsed = (finish.tv_sec - t_start.tv_sec);
    elapsed += (finish.tv_nsec - t_start.tv_nsec) / 1000000000.0;

    printf("Time used was %fms for multi-threaded. \n", elapsed * 1000);
        
    free_matrix(m_1);
    free_matrix(m_2);
    free_matrix(ans_single);
    free_matrix(ans_multi);
}