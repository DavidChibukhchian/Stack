#include "Stack.h"

int main()
{
    Stack stk1 = {};

    stackCtor(&stk1, 2);
    stackDisplay(&stk1);

    stackPush(&stk1, 7);

    stackPush(&stk1, 8);
    stackDisplay(&stk1);

    stackPush(&stk1, 9);
    stackDisplay(&stk1);

    elem_t top1 = 0;
    stackTop(&stk1, &top1);

    elem_t top2 = 0;
    stackPop(&stk1);
    stackDisplay(&stk1);

    stackPop(&stk1, &top2);
    stackDisplay(&stk1);

    stackDtor(&stk1);

    return 0;
}