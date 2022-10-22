#ifndef TASK3_STACK_H
#define TASK3_STACK_H

#include <stdio.h>

typedef double elem_t;
typedef long long int canary_t;

enum stack_status
{
    ACTIVE,
    INACTIVE
};

struct Stack
{
    canary_t left_struct_canary;

    stack_status status;
    size_t size;
    size_t capacity;
    elem_t* first_elem;
    elem_t* top_elem;
    FILE* logfile;

    canary_t* left_buff_canary;
    canary_t* right_buff_canary;

    int errors;
    unsigned int buffer_hash;
    unsigned int struct_hash;

    canary_t right_struct_canary;
};

int stackCtor (Stack* stk, size_t init_capacity = 1);

int stackPush (Stack* stk, elem_t value);

int stackPop  (Stack* stk, elem_t* top = nullptr);

int stackTop  (Stack* stk, elem_t* top);

int stackDtor (Stack* stk);

int stackDisplay (const Stack* stk);

#endif //TASK3_STACK_H