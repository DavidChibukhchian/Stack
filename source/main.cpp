#include <stdio.h>
#include "Stack.h"

int main()
{
    Stack stk1 = {};

    stackCtor(&stk1, 2);

    stackPush(&stk1, 7);
    stackPush(&stk1, 8);
    stackPush(&stk1, 9);

    stackPop(&stk1);
    stackDisplay(&stk1);

    elem_t top = 0;
    stackTop(&stk1, &top);

    stackDtor(&stk1);

    return 0;
}
