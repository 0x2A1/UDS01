#include "Stack.h"

bool stack_init(U8_Stack *s, uint8_t *buffer, size_t capacity)
{
    if (s == NULL || buffer == NULL || capacity == 0)
        return false;
    s->data = buffer;
    s->capacity = capacity;
    s->top = 0;
    return true;
}

void stack_clear(U8_Stack *s)
{
    if (s != NULL)
        s->top = 0;
}

bool stack_push(U8_Stack *s, uint8_t value)
{
    if (s == NULL || s->top >= s->capacity)
        return false;
    s->data[s->top++] = value;
    return true;
}

bool stack_pop(U8_Stack *s, uint8_t *out)
{
    if (s == NULL || s->top == 0)
        return false;
    s->top--;
    if (out != NULL)
        *out = s->data[s->top];
    return true;
}

bool stack_peek(const U8_Stack *s, uint8_t *out)
{
    if (s == NULL || s->top == 0 || out == NULL)
        return false;
    *out = s->data[s->top - 1];
    return true;
}

bool stack_is_empty(const U8_Stack *s)
{
    return (s == NULL || s->top == 0);
}

size_t stack_size(const U8_Stack *s)
{
    return (s == NULL) ? 0 : s->top;
}

size_t stack_capacity(const U8_Stack *s)
{
    return (s == NULL) ? 0 : s->capacity;
}

bool stack_is_full(const U8_Stack *s)
{
    return (s == NULL || s->top >= s->capacity);
}

/* ========== 测试示例（编译时定义 TEST 宏启用） ========== */
#ifdef TEST
#include <stdio.h>

int main(void)
{
    // 使用静态数组作为栈的存储（例如容量 10）
    uint8_t buffer[10];
    U8_Stack st;

    if (!stack_init(&st, buffer, sizeof(buffer)))
    {
        fprintf(stderr, "初始化失败\n");
        return 1;
    }

    printf("压入 0~9: ");
    for (uint8_t i = 0; i < 10; i++)
    {
        if (stack_push(&st, i))
            printf("%u ", i);
        else
            printf("[%u失败] ", i);
    }
    printf("\n当前大小: %zu, 容量: %zu\n", stack_size(&st), stack_capacity(&st));

    // 尝试再压入一个（应失败）
    if (!stack_push(&st, 99))
        printf("压入 99 失败（栈已满）\n");

    uint8_t top_val;
    if (stack_peek(&st, &top_val))
        printf("栈顶元素: %u\n", top_val);

    printf("弹出所有元素: ");
    while (!stack_is_empty(&st))
    {
        uint8_t val;
        stack_pop(&st, &val);
        printf("%u ", val);
    }
    printf("\n");

    // 清空栈（实际上已经空了）
    stack_clear(&st);
    printf("清空后大小: %zu\n", stack_size(&st));

    return 0;
}
#endif // TEST