/********************************************************************************
 * @copyright   Copyright (c)
 * @file        LED.h
 * @author      ouqingfeng
 * @date        2026/06/16
 * @brief
 * @attention  注意事项--
 * @ChangeLogs
 *  Date             Author          Notes
 *  2026/06/16       ouqingfeng          First version
 ********************************************************************************/
#ifndef _LED_H_
#define _LED_H_
//---------------------------------文件包含---------------------------------------
#include "../../Core/Global.h"
#include "../../HAL/include/hal_gpio.h"
//---------------------------------本文内宏变量定义--------------------------------
//---------------------------------本文内宏命令定义--------------------------------
//---------------------------------本文内数据结构定义------------------------------
// LED工作模式
typedef enum
{
    LED_MODE_OFF,  // 常灭
    LED_MODE_ON,   // 常亮
    LED_MODE_BLINK // 闪烁
} LED_MODE_E;

// LED物理状态
typedef enum
{
    LED_STATE_OFF = 0,
    LED_STATE_ON = 1
} LED_STATE_E;
// LED控制器
typedef struct
{
    LED_STATE_E State; // 当前LED状态

    LED_MODE_E mode;         // 当前模式
    uint16_t period_ms;      // 闪烁周期(ms)
    uint16_t blink_on_time;  // 亮持续时间(ms)
    uint16_t blink_off_time; // 灭持续时间(ms)
    uint64_t last_time;      // 上一次状态翻转的时间戳
    HAL_GPIO_S io;           // 硬件io号
} LED_S;
//---------------------------------本文件内变量声明--------------------------------
extern LED_S led1;
//---------------------------------本文件内函数声明--------------------------------
void LED_Init(LED_S *pLed, float freq_hz, float duty_cycle);
void LED_SetMode(LED_S *pLed, LED_MODE_E mode);
void LED_SetState(LED_S *pLed, LED_STATE_E state);
void LED_SetBlink(LED_S *pLed, float freq_hz, float duty_cycle);
void LED_Task(LED_S *pLed, uint64_t current_time);
#endif /*LED_H_*/
