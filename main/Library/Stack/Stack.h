#ifndef UINT8_STACK_H
#define UINT8_STACK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * uint8_t 静态栈结构体
 * - data: 指向用户提供的缓冲区（不负责分配/释放）
 * - capacity: 缓冲区可容纳的最大元素个数
 * - top: 当前栈顶索引（即元素个数）
 */
typedef struct
{
    uint8_t *data;
    size_t capacity;
    size_t top;
} U8_Stack;

/**
 * 初始化栈（使用用户提供的缓冲区）
 * @param s        栈指针
 * @param buffer   外部提供的 uint8_t 数组（必须至少能容纳 capacity 个元素）
 * @param capacity 缓冲区容量（最大元素个数）
 * @return true 成功，false 失败（参数无效）
 */
bool stack_init(U8_Stack *s, uint8_t *buffer, size_t capacity);

/**
 * 重置栈（清空所有元素，不释放内存）
 * @param s 栈指针
 */
void stack_clear(U8_Stack *s);

/**
 * 入栈
 * @param s     栈指针
 * @param value 要压入的值
 * @return true 成功，false 失败（栈已满）
 */
bool stack_push(U8_Stack *s, uint8_t value);

/**
 * 出栈
 * @param s   栈指针
 * @param out 存储弹出的值（若为 NULL 则仅弹出）
 * @return true 成功，false 失败（栈为空）
 */
bool stack_pop(U8_Stack *s, uint8_t *out);

/**
 * 查看栈顶元素（不弹出）
 * @param s   栈指针
 * @param out 存储栈顶值（不能为 NULL）
 * @return true 成功，false 失败（栈为空或 out 为 NULL）
 */
bool stack_peek(const U8_Stack *s, uint8_t *out);

/**
 * 判断栈是否为空
 */
bool stack_is_empty(const U8_Stack *s);

/**
 * 获取栈中元素个数
 */
size_t stack_size(const U8_Stack *s);

/**
 * 获取栈的容量（最大元素个数）
 */
size_t stack_capacity(const U8_Stack *s);

/**
 * 检查栈是否已满
 */
bool stack_is_full(const U8_Stack *s);

#endif // UINT8_STACK_H