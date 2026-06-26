/********************************************************************************
 * @copyright   Copyright (c)
 * @file        hal_timer.h
 * @author      ouqingfeng
 * @date        2026/06/22
 * @brief       硬件抽象层 - 通用定时器接口
 * @attention   本接口基于 bsp_time.c 实现，向上提供统一的操作函数。
 *              最多支持 TIMER_MAX_NUM 个定时器实例。
 *              接口采用"分配-初始化-启动"分离的设计模式。
 * @ChangeLogs
 *  Date             Author          Notes
 *  2026/06/22       ouqingfeng      First version
 *  2026/06/25       ouqingfeng      优化接口注释，增加运行状态查询函数
 ********************************************************************************/
#ifndef _HAL_TIMER_H_
#define _HAL_TIMER_H_
//---------------------------------文件包含---------------------------------------
#include "../../Core/Global.h" // 假定提供 uint32_t, bool, int8_t, uint64_t
//---------------------------------本文内宏变量定义--------------------------------
#define TIMER_MAX_NUM 4 ///< 系统最大可用的定时器实例数量（静态池大小）
//---------------------------------本文内宏命令定义--------------------------------
//---------------------------------本文内数据结构定义------------------------------
/* 不透明结构体：外部只能通过指针访问，实现细节隐藏在 bsp_time.c 中 */
struct TIMER_S;
typedef struct TIMER_S TIMER_S;
typedef void (*TIMER_CALLBACK)(void *arg); // 定时器回调函数类型

/**
 * @brief 定时器配置结构体
 */
typedef struct
{
    uint32_t freq_hz; // 期望的定时器中断频率（Hz）。实际报警计数 = 1,000,000 / freq_hz
    bool count_up;    // true = 向上计数，false = 向下计数
    bool auto_reload; // true = 触发报警后自动重载计数值，false = 单次报警后停止
} TIMER_CONFIG_S;
//---------------------------------本文件内变量声明--------------------------------
//---------------------------------本文件内函数声明--------------------------------
/*********************************************************************************
 * @name          timer_alloc
 * @brief         从静态池分配一个定时器实例（仅标记槽位，不初始化硬件）
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：代表含义
 * @return         指向空闲 TIMER_S 的指针，若全满则返回 NULL
 *********************************************************************************/
TIMER_S *timer_alloc(void);

/*********************************************************************************
 * @name          timer_free
 * @brief         归还定时器槽位到静态池（应在 deinit 之后调用）
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         待释放的实例指针
 * @return        返回值：代表含义
 *********************************************************************************/
void timer_free(TIMER_S *pTimer);

/*********************************************************************************
 * @name          timer_init
 * @brief         函数功能简述
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：由 timer_alloc() 获得的实例指针
 * @param         Config：定时器参数配置
 * @param         Callback：中断回调函数
 * @param         CallbackArg：中断回调函数传入的参数
 * @return        返回值：0 成功，-1 失败（参数无效或硬件操作错误）
 *********************************************************************************/
int8_t timer_init(TIMER_S *pTimer, TIMER_CONFIG_S Config, TIMER_CALLBACK Callback, void *CallbackArg);

/*********************************************************************************
 * @name          timer_start
 * @brief         使能并启动定时器计数
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：已初始化但未启动的实例指针
 * @return        返回值：代表含义
 *********************************************************************************/
void timer_start(TIMER_S *pTimer);

/*********************************************************************************
 * @name          timer_stop
 * @brief         停止定时器并禁用硬件
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：正在运行中的实例指针
 * @return        返回值：代表含义
 *********************************************************************************/
void timer_stop(TIMER_S *pTimer);

/*********************************************************************************
 * @name          timer_get_counter
 * @brief         获取定时器当前原始计数值（单位：tick）
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：已初始化的实例指针
 * @return        返回值：当前计数值，失败返回 0
 *********************************************************************************/
uint64_t timer_get_counter(TIMER_S *pTimer);

/*********************************************************************************
 * @name          timer_deinit
 * @brief         反初始化定时器：停止、删除硬件资源，并标记槽位为空闲
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：已初始化的实例指针
 * @return        返回值：代表含义
 *********************************************************************************/
void timer_deinit(TIMER_S *pTimer);

/*********************************************************************************
 * @name          timer_is_running
 * @brief         查询定时器是否正在运行
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：定时器实例指针
 * @return        返回值：true 运行中，false 未运行或指针无效
 *********************************************************************************/
bool timer_is_running(TIMER_S *pTimer);
#endif /*HAL_TIMER_H_*/

/* 使用示例 */

// // 1. 分配定时器
// TIMER_S *my_timer = timer_alloc();

// // 2. 配置并初始化（1kHz 中断，向上计数，自动重载）
// TIMER_CONFIG_S config = {
//     .freq_hz = 1000,
//     .count_up = true,
//     .auto_reload = true};
// if (timer_init(my_timer, config, timercallback, timercallbackagv) != 0)
// {
//     timer_free(my_timer);
//     return;
// }

// // 3. 启动定时器
// timer_start(my_timer);

// // ... 使用中 ...

// // 4. 停止、反初始化并释放
// timer_stop(my_timer);
// timer_deinit(my_timer);
// timer_free(my_timer);
//