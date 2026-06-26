/********************************************************************************
 * @copyright   Copyright (c)
 * @file        hal_gpio.h
 * @author      ouqingfeng
 * @date        2026/06/05
 * @brief       GPIO通用驱动头文件，提供引脚定义、配置枚举及操作接口
 * @attention   1. 依赖全局头文件 Global.h，需保证路径正确
 *              2. 中断回调函数建议使用弱函数或全局函数，避免栈溢出
 *              3. 不同MCU端口寄存器地址需按硬件实际映射赋值
 * @ChangeLogs
 *  Date             Author          Notes
 *  2026/06/05       ouqingfeng          First version
 ********************************************************************************/
#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_
//---------------------------------文件包含---------------------------------------
#include "../../Core/Global.h"
//---------------------------------本文内宏变量定义--------------------------------
//---------------------------------本文内宏命令定义--------------------------------
//---------------------------------本文内数据结构定义------------------------------
/**
 * @brief  GPIO工作模式枚举
 */
typedef enum
{
    HAL_GPIO_MODE_INPUT = 0,    // 输入模式
    HAL_GPIO_MODE_OUTPUT_PP,    // 推挽输出模式
    HAL_GPIO_MODE_OUTPUT_OP,    // 开漏输出模式
    HAL_GPIO_MODE_ALT_FUNCTION, // 外设功能复用模式
    HAL_GPIO_MODE_ANALOG        // 模拟输入模式(ADC采集)
} HAL_GPIO_MODE_E;

/**
 * @brief  GPIO上下拉配置枚举
 */
typedef enum
{
    HAL_GPIO_PULL_NONE = 0, // 无上下拉
    HAL_GPIO_PULL_UP = 1,   // 上拉使能
    HAL_GPIO_PULL_DOWN = 2  // 下拉使能
} HAL_GPIO_PULL_E;

typedef enum
{
    GPIO_POLAR_NORMAL = 0, // 正常极性：逻辑电平 = 硬件物理电平
    GPIO_POLAR_INVERT = 1  // 反转极性：逻辑电平 = 硬件电平取反
} HAL_GPIO_POLAR_E;

/**
 * @brief  GPIO输出速度等级枚举
 */
typedef enum
{
    GPIO_SPEED_LOW = 0,      // 低速
    GPIO_SPEED_MEDIUM = 1,   // 中速
    GPIO_SPEED_HIGH = 2,     // 高速
    GPIO_SPEED_VERY_HIGH = 3 // 超高速
} HAL_GPIO_SPEED_E;

/**
 * @brief  GPIO中断触发方式枚举
 */
typedef enum
{
    GPIO_IRQ_TRIGGER_NONE = 0,    // 不开启中断
    GPIO_IRQ_TRIGGER_RISING = 1,  // 上升沿触发
    GPIO_IRQ_TRIGGER_FALLING = 2, // 下降沿触发
    GPIO_IRQ_TRIGGER_BOTH = 3     // 双边沿触发
} HAL_GPIO_IRQ_TRIGGER_E;

/**
 * @brief  GPIO完整配置参数结构体
 * @note   包含模式、上下拉、速度、中断及回调配置
 */
typedef struct
{
    HAL_GPIO_MODE_E Mode;              // GPIO工作模式
    HAL_GPIO_PULL_E Pull;              // 上下拉配置
    HAL_GPIO_SPEED_E Speed;            // 输出速度配置
    HAL_GPIO_IRQ_TRIGGER_E IrqTrigger; // 中断触发方式
    void (*IrqCallback)(void *arg);    // 中断服务回调函数
    void *IrqArg;                      // 传给回调函数的自定义参数
} HAL_GPIO_CONFIG_S;

/**
 * @brief  GPIO引脚实体结构体
 * @note   用于唯一标识一个GPIO引脚（端口+引脚号）
 */
typedef struct
{
    void *Port;             // GPIO端口寄存器基地址指针
    uint8_t Pin;            // 引脚编号(0~15)
    HAL_GPIO_POLAR_E Polar; // 引脚电平极性
} HAL_GPIO_S;
//---------------------------------本文件内变量声明--------------------------------
//---------------------------------本文件内函数声明--------------------------------

/**
 * @brief  GPIO引脚始化
 * @param  pin: 待操作GPIO引脚指针
 * @param  cfg: GPIO配置参数指针
 * @retval 无
 */
void HAL_GpioInit(const HAL_GPIO_S *pin, const HAL_GPIO_CONFIG_S *cfg);

/**
 * @brief  GPIO引脚反初始化，恢复默认状态
 * @param  pin: 待操作GPIO引脚指针
 * @retval 无
 */
void HAL_GpioDeinit(const HAL_GPIO_S *pin);

/**
 * @brief  读取GPIO引脚电平状态
 * @param  pin: 待读取GPIO引脚指针
 * @retval 0:低电平  1:高电平
 */
uint8_t HAL_GpioRead(const HAL_GPIO_S *pin);

/**
 * @brief  设置GPIO引脚输出电平
 * @param  pin: 待操作GPIO引脚指针
 * @param  value: 0-低电平  1-高电平
 * @retval 无
 */
void HAL_GpioWrite(const HAL_GPIO_S *pin, uint8_t value);

/**
 * @brief  GPIO引脚电平翻转
 * @param  pin: 待翻转GPIO引脚指针
 * @retval 无
 */
void HAL_GpioToggle(const HAL_GPIO_S *pin);

/**
 * @brief  使能GPIO外部中断
 * @param  pin: 目标GPIO引脚指针
 * @retval 0:失败  1:成功
 */
uint8_t HAL_GpioIrqEnable(const HAL_GPIO_S *pin);

/**
 * @brief  关闭GPIO外部中断
 * @param  pin: 目标GPIO引脚指针
 * @retval 无
 */
void HAL_GpioIrqDisable(const HAL_GPIO_S *pin);

#endif /*HAL_GPIO_H_*/
