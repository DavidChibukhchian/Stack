#include "Stack.h"

int main()
{
    Stack stk1 = {};

    stackCtor(&stk1, 2);

    stackPush(&stk1, 7);
    stackPush(&stk1, 8);
    stackPush(&stk1, 9);

    elem_t top1 = 0;
    stackTop(&stk1, &top1);
    
    stackPop(&stk1);

    elem_t top2 = 0;
    stackPop(&stk1, &top2);

    stackDtor(&stk1);

    return 0;
}
