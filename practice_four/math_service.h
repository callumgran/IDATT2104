#ifndef MATH_SERVICE_H
#define MATH_SERVICE_H

#ifdef PORT
#undef PORT
#endif

#define PORT 8333
#define IP "10.24.2.29"
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
