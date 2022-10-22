#include <stdio.h>
#include <malloc.h>
#include "Stack.h"

//----------------------------------------------------------------------------------------------------------------------

static const canary_t CANARY = 0xABCDE12345;
static const elem_t   POISON = 0x111ABCBA111;
static const int      MULTIPLIER = 2;
static const size_t   BUFFER_SIZE_MAX = 1e7;

//----------------------------------------------------------------------------------------------------------------------

static const size_t SIZE_OF_STRUCT =  1 * sizeof(stack_status)
                                    + 2 * sizeof(size_t)
                                    + 2 * sizeof(elem_t)
                                    + 2 * sizeof(canary_t)
                                    + 1 * sizeof(FILE*);

//----------------------------------------------------------------------------------------------------------------------

#define SIZE_OF_BUFF          (2 * sizeof(canary_t) + stk->capacity * sizeof(elem_t))
#define SIZE_OF_INIT_BUFF     (2 * sizeof(canary_t) + init_capacity * sizeof(elem_t))
#define SIZE_OF_EXPANDED_BUFF (sizeof(elem_t) * ((stk->capacity) * MULTIPLIER) + 2 * sizeof(canary_t))
#define SIZE_OF_SHRANK_BUFF   (sizeof(elem_t) * ((stk->capacity) / MULTIPLIER) + 2 * sizeof(canary_t))

//----------------------------------------------------------------------------------------------------------------------

enum Resize_mode
{
    EXPAND,
    SHRINK
};

//----------------------------------------------------------------------------------------------------------------------

enum Errors
{
    Stack_Pointer_Is_Null    = -1,
    Failed_To_Create_Logfile = -2,
    Logfile_Is_Not_Found     = -3,
    Stack_Is_Destructed      = -4,
    Mem_Pointer_Is_Null      = -5,
    Mem_Size_Is_Zero         = -6,

    Displayed_Successfully   =  1,
    Destructed_Successfully  =  2,
    Printed_Successfully     =  3
};

//----------------------------------------------------------------------------------------------------------------------

enum Verification_Errors
{
    Stack_Is_OK                    =  0,
    Size_Is_Over_Capacity          =  1 << 0,
    Pointer_To_TopElem_Is_Null     =  1 << 1,
    Incorrect_Pointer_To_TopElem   =  1 << 2,
    Stack_Is_Overflow              =  1 << 3,
    Left_Buff_Canary_Is_Damaged    =  1 << 4,
    Right_Buff_Canary_Is_Damaged   =  1 << 5,
    Left_Struct_Canary_Is_Damaged  =  1 << 6,
    Right_Struct_Canary_Is_Damaged =  1 << 7,
    Incorrect_Buff_Hash            =  1 << 8,
    Incorrect_Struct_Hash          =  1 << 9,
    Stack_Is_Empty                 =  1 << 10,
    Failed_To_Resize_Stack         =  1 << 11,
    Failed_To_Create_Stack_Buffer  =  1 << 12
};

//----------------------------------------------------------------------------------------------------------------------

static int stack_is_destructed(const Stack* stk)
{
    if (stk->status == INACTIVE)
    {
        printf("\nStack is destructed.\n");
        return Stack_Is_Destructed;
    }
    else
        return Stack_Is_OK;
}

//----------------------------------------------------------------------------------------------------------------------

static int stack_pointer_is_null(const Stack* stk)
{
    if (stk == nullptr)
    {
        printf("\nPointer to stack is null.\n");
        return Stack_Pointer_Is_Null;
    }
    else
        return Stack_Is_OK;
}

//----------------------------------------------------------------------------------------------------------------------

static unsigned int hash_FAQ6(const void* mem_pointer, size_t mem_size)
{
    if (mem_pointer == nullptr)
        return Mem_Pointer_Is_Null;
    if (mem_size == 0)
        return Mem_Size_Is_Zero;

    unsigned int hash = 0;
    char* ptr = (char*) mem_pointer;

    for (size_t i = 0; i < mem_size; i++)
    {
        hash += (unsigned char) ptr[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

//----------------------------------------------------------------------------------------------------------------------

static void print_buffer_to_logfile(const Stack* stk)
{
    for (size_t i = 0; i < stk->capacity; i++)
    {
        if (stk->first_elem[i] == POISON)
        {
            fprintf (stk->logfile, "[P] ");
        }
        else
        {
            fprintf (stk->logfile, "[%lg] ", stk->first_elem[i]);
        }
    }

    fprintf(stk->logfile, "\n\nsize = %zu",       stk->size);
    fprintf(stk->logfile,   "\ncapacity = %zu\n", stk->capacity);
}

//----------------------------------------------------------------------------------------------------------------------

static int print_stack_to_logfile(Stack* stk)
{
    if (stk->logfile == nullptr)
    {
        printf("\nLogfile is not found.\n");
        return Logfile_Is_Not_Found;
    }

    print_buffer_to_logfile(stk);

    if (stk->errors & (Size_Is_Over_Capacity)) {
        fprintf(stk->logfile, "ERROR: Buffer size is over capacity\n");
    }
    if (stk->errors & (Pointer_To_TopElem_Is_Null)) {
        fprintf(stk->logfile, "ERROR: Pointer to top element is null\n");
    }
    if (stk->errors & (Incorrect_Pointer_To_TopElem)) {
        fprintf(stk->logfile, "ERROR: Incorrect pointer to top element\n");
    }
    if (stk->errors & (Stack_Is_Overflow)) {
        fprintf(stk->logfile, "ERROR: Stack is overflow\n");
    }
    if (stk->errors & (Left_Buff_Canary_Is_Damaged)) {
        fprintf(stk->logfile, "ERROR: Left buffer canary is damaged\n");
    }
    if (stk->errors & (Right_Buff_Canary_Is_Damaged)) {
        fprintf(stk->logfile, "ERROR: Right buffer canary is damaged\n");
    }
    if (stk->errors & (Left_Struct_Canary_Is_Damaged)) {
        fprintf(stk->logfile, "ERROR: Left structure canary is damaged\n");
    }
    if (stk->errors & (Right_Struct_Canary_Is_Damaged)) {
        fprintf(stk->logfile, "ERROR: Right structure canary is damaged\n");
    }
    if (stk->errors & (Incorrect_Buff_Hash)) {
        fprintf(stk->logfile, "ERROR: Incorrect buffer hash\n");
    }
    if (stk->errors & (Incorrect_Struct_Hash)) {
        fprintf(stk->logfile, "ERROR: Incorrect structure hash\n");
    }
    if (stk->errors & (Stack_Is_Empty)) {
        fprintf(stk->logfile, "ERROR: Stack is empty\n");
    }
    if (stk->errors & (Failed_To_Resize_Stack)) {
        fprintf(stk->logfile, "ERROR: Failed to resize stack\n");
    }
    if (stk->errors & (Failed_To_Create_Stack_Buffer)) {
        fprintf(stk->logfile, "ERROR: Failed to create stack buffer\n");
    }

    fprintf(stk->logfile, "-------------------------------------------\n\n");

    return Printed_Successfully;
}

//----------------------------------------------------------------------------------------------------------------------

static int stack_verificator(Stack* stk)
{
    if (stack_pointer_is_null(stk))
        return Stack_Pointer_Is_Null;
    if (stack_is_destructed(stk))
        return Stack_Is_Destructed;

    if (stk->size > stk->capacity)
    {
        stk->errors |= Size_Is_Over_Capacity;
    }
    if (stk->top_elem == nullptr)
    {
        stk->errors |= Pointer_To_TopElem_Is_Null;
    }

    if (stk->size == 0)
    {
        if (stk->top_elem != stk->first_elem)
        {
            stk->errors |= Incorrect_Pointer_To_TopElem;
        }
    }
    else
    {
        if (stk->top_elem != (stk->first_elem + stk->size - 1))
        {
            stk->errors |= Incorrect_Pointer_To_TopElem;
        }
    }

    if (stk->capacity > BUFFER_SIZE_MAX)
    {
        stk->errors |= Stack_Is_Overflow;
    }

    if (*stk->left_buff_canary != CANARY)
    {
        stk->errors |= Left_Buff_Canary_Is_Damaged;
    }
    if (*stk->right_buff_canary != CANARY)
    {
        stk->errors |= Right_Buff_Canary_Is_Damaged;
    }
    if (stk->left_struct_canary != CANARY)
    {
        stk->errors |= Left_Struct_Canary_Is_Damaged;
    }
    if (stk->right_struct_canary != CANARY)
    {
        stk->errors |= Right_Struct_Canary_Is_Damaged;
    }

    unsigned int recalculated_buffer_hash = hash_FAQ6(stk->left_buff_canary, SIZE_OF_BUFF);
    unsigned int recalculated_struct_hash = hash_FAQ6(&stk->status, SIZE_OF_STRUCT);

    if (recalculated_buffer_hash != stk->buffer_hash)
    {
        stk->errors |= Incorrect_Buff_Hash;
    }
    if (recalculated_struct_hash != stk->struct_hash)
    {
        stk->errors |= Incorrect_Struct_Hash;
    }

    if (stk->errors != 0)
    {
        print_stack_to_logfile(stk);
    }

    int result = stk->errors;
    stk->errors = 0;
    return result;
}

//----------------------------------------------------------------------------------------------------------------------

static void make_struct_non_valid(Stack* stk)
{
    stk->size     = SIZE_MAX;
    stk->capacity = SIZE_MAX;

    stk->first_elem = nullptr;
    stk->top_elem   = nullptr;

    stk-> left_struct_canary = LONG_LONG_MAX;
    stk->right_struct_canary = LONG_LONG_MAX;
}

//----------------------------------------------------------------------------------------------------------------------

static void make_buffer_non_valid(Stack* stk)
{
    *(stk-> left_buff_canary) = LONG_LONG_MAX;
    *(stk->right_buff_canary) = LONG_LONG_MAX;

    for (size_t i = 0; i < stk->capacity; i++)
    {
        stk->first_elem[i] = POISON;
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void put_poison_to_buffer(Stack* stk)
{
    elem_t* ptr = stk->top_elem;
    if (stk->size != 0)
    {
        ptr++;
    }
    for (size_t i = 0; i < stk->capacity - stk->size; i++)
    {
        ptr[i] = POISON;
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void recalculate_buffer_hash(Stack* stk)
{
    stk->buffer_hash = hash_FAQ6(stk->left_buff_canary, SIZE_OF_BUFF);
}

//----------------------------------------------------------------------------------------------------------------------

static void recalculate_struct_hash(Stack* stk)
{
    stk->struct_hash = hash_FAQ6(&stk->status, SIZE_OF_STRUCT);
}

//----------------------------------------------------------------------------------------------------------------------

static void set_buff_canaries(Stack* stk)
{
    *(stk->left_buff_canary)  = CANARY;

    stk->right_buff_canary = (canary_t*) (stk->first_elem + stk->capacity);
    *(stk->right_buff_canary) = CANARY;
}

//----------------------------------------------------------------------------------------------------------------------

static void set_struct_canaries(Stack* stk)
{
    stk->left_struct_canary  = CANARY;
    stk->right_struct_canary = CANARY;
}

//----------------------------------------------------------------------------------------------------------------------

int stackCtor(Stack* stk, size_t init_capacity)
{
   if (stack_pointer_is_null(stk))
       return Stack_Pointer_Is_Null;

   FILE* logfile = fopen("stack_logfile.txt", "w");
   if (logfile == nullptr)
   {
       printf("\nFailed to create a logfile.\n");
       fclose(logfile);
       return Failed_To_Create_Logfile;
   }
   stk->logfile = logfile;

   if (init_capacity > BUFFER_SIZE_MAX)
   {
       fprintf(stk->logfile, "\nToo big size to create a stack.\n");
       fclose(logfile);
       return Failed_To_Create_Stack_Buffer;
   }

   stk->left_buff_canary = (canary_t*)calloc (SIZE_OF_INIT_BUFF, sizeof(char));
   if (stk->left_buff_canary == nullptr)
   {
       fprintf(stk->logfile, "\nFailed to create stack buffer.\n");
       fclose(logfile);
       return Failed_To_Create_Stack_Buffer;
   }

   stk->status   = ACTIVE;
   stk->size     = 0;
   stk->errors   = 0;
   stk->capacity = init_capacity;

   stk->first_elem = (elem_t*)(stk->left_buff_canary + 1);
   stk->top_elem   = stk->first_elem;

   put_poison_to_buffer(stk);

   set_buff_canaries(stk);
   set_struct_canaries(stk);

   recalculate_buffer_hash(stk);
   recalculate_struct_hash(stk);

   return stack_verificator(stk);
}

//----------------------------------------------------------------------------------------------------------------------

static int stack_resize(Stack* stk, Resize_mode mode)
{
    size_t new_buff_size = 0;
    size_t old_capacity  = stk->capacity; // to get correct structure hash when verification

    if (mode == EXPAND)
    {
        new_buff_size = SIZE_OF_EXPANDED_BUFF;
        stk->capacity *= MULTIPLIER;
    }
    else // if (mode == SHRINK)
    {
        new_buff_size = SIZE_OF_SHRANK_BUFF;
        stk->capacity /= MULTIPLIER;
    }

    canary_t* ptr_to_new_buffer = (canary_t*)realloc (stk->left_buff_canary, new_buff_size);
    if (ptr_to_new_buffer == nullptr)
    {
        stk->capacity = old_capacity;   // to get correct structure hash when verification
        return Failed_To_Resize_Stack;
    }

    stk->left_buff_canary = ptr_to_new_buffer;

    stk->first_elem = (elem_t*) (stk->left_buff_canary + 1);
    stk->top_elem   = stk->first_elem + stk->size - 1;

    put_poison_to_buffer(stk);

    set_buff_canaries(stk);

    return Stack_Is_OK;
}

//----------------------------------------------------------------------------------------------------------------------

int stackPush(Stack* stk, elem_t value)
{
    if (stack_pointer_is_null(stk))
        return Stack_Pointer_Is_Null;
    if (stack_is_destructed(stk))
        return Stack_Is_Destructed;

    int errors = stack_verificator(stk);
    if (errors != Stack_Is_OK)
        return errors;

    if (stk->capacity == stk->size)
    {
        int res = stack_resize(stk, EXPAND);
        if (res == Failed_To_Resize_Stack)
        {
            stk->errors |= Failed_To_Resize_Stack;
            return stack_verificator(stk);
        }
    }

    if (stk->size != 0)
    {
        (stk->top_elem)++;
    }
    *(stk->top_elem) = value;
    stk->size++;

    recalculate_buffer_hash(stk);
    recalculate_struct_hash(stk);

    return stack_verificator(stk);
}

//----------------------------------------------------------------------------------------------------------------------

int stackPop(Stack* stk, elem_t* top)
{
    if (stack_pointer_is_null(stk))
        return Stack_Pointer_Is_Null;
    if (stack_is_destructed(stk))
        return Stack_Is_Destructed;

    int errors = stack_verificator(stk);
    if (errors != Stack_Is_OK)
        return errors;

    if (stk->size >= 1)
    {
        if (top != nullptr)
        {
            *top = *stk->top_elem;
        }
        *(stk->top_elem) = POISON;
        if (stk->size != 1)
        {
            (stk->top_elem)--;
        }
        (stk->size)--;
    }
    else
    {
        stk->errors |= Stack_Is_Empty;
        return stack_verificator(stk);
    }

    recalculate_buffer_hash(stk);
    recalculate_struct_hash(stk);

    if ((stk->size <= (stk->capacity)/(MULTIPLIER*MULTIPLIER)) && (stk->size != 0))
    {
        int res = stack_resize(stk, SHRINK);
        if (res == Failed_To_Resize_Stack)
        {
            stk->errors |= Failed_To_Resize_Stack;
            return stack_verificator(stk);
        }
    }

    recalculate_buffer_hash(stk);
    recalculate_struct_hash(stk);

    return stack_verificator(stk);
}

//----------------------------------------------------------------------------------------------------------------------

int stackTop(Stack* stk, elem_t* top)
{
    if (stack_pointer_is_null(stk))
        return Stack_Pointer_Is_Null;
    if (stack_is_destructed(stk))
        return Stack_Is_Destructed;

    int errors = stack_verificator(stk);
    if (errors != Stack_Is_OK)
        return errors;

    if (stk->size != 0)
    {
        *top = *stk->top_elem;
    }
    else
    {
        stk->errors |= Stack_Is_Empty;
    }

    return stack_verificator(stk);
}

//----------------------------------------------------------------------------------------------------------------------

int stackDtor(Stack* stk)
{
    if (stack_pointer_is_null(stk))
        return Stack_Pointer_Is_Null;
    if (stack_is_destructed(stk))
        return Stack_Is_Destructed;

    int errors = stack_verificator(stk);
    if (errors != Stack_Is_OK)
        return errors;

    stk->status = INACTIVE;

    make_buffer_non_valid(stk);
    make_struct_non_valid(stk);

    fclose(stk->logfile);
    free(stk->left_buff_canary);

    return Destructed_Successfully;
}

//----------------------------------------------------------------------------------------------------------------------

int stackDisplay(const Stack* stk)
{
    if (stack_pointer_is_null(stk))
        return Stack_Pointer_Is_Null;
    if (stack_is_destructed(stk))
        return Stack_Is_Destructed;

    for (size_t i = 0; i < stk->capacity; i++)
    {
        if (stk->first_elem[i] == POISON)
            printf("[P] ");
        else
            printf("[%lg] ", stk->first_elem[i]);
    }
    printf("\nsize = %zu",      stk->size);
    printf("\ncapacity = %zu",  stk->capacity);

//    printf("\nbuffer hash: %u", stk->buffer_hash);
//    printf("\nstruct hash: %u", stk->struct_hash);

    printf("\n---------------------------------------\n");

    return Displayed_Successfully;
}

//----------------------------------------------------------------------------------------------------------------------