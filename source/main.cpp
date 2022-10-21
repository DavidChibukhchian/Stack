#include "Stack.h"

int main()
{
    Stack stk1 = {};

    stackCtor(&stk1, 1);

    stackPush(&stk1, 7);
    stackPush(&stk1, 8);

    elem_t top = 0;
    stackTop(&stk1, &top);

    stackPop(&stk1);

    stackDtor(&stk1);

    return 0;
}