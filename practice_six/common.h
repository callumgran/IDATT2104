#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Structs */

#define HTTP_PORT 8090
#define WS_PORT 8091

#define LOG(...) \
    do { \
        printf("\033[0;33m[LOG]: "); \
        printf(__VA_ARGS__); \
        printf("\033[0m\n"); \
    } while (0) \

#define LOG_ERR(...) \
    do { \
        fprintf(stderr, "\033[0;31m[LOG]: "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\033[0m\n"); \
    } while (0) \

#endif