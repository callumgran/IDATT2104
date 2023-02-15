#ifndef MATH_SERVICE_H
#define MATH_SERVICE_H

#ifdef PORT
#undef PORT
#endif

#define PORT 8333
#define IP "127.0.0.1"
#define MSG_MAX 64

enum arithmetic_op {
    ADD = '+',
    SUB = '-',
    MUL = '*',
    DIV = '/',
    MOD = '%'
};

struct arithmetic_t {
    int a;
    int b;
    enum arithmetic_op op;
};

#endif