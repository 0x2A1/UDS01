/********************************************************************************
 * @copyright   Copyright (c)
 * @file        LED.c
 * @author      ouqingfeng
 * @date        2026/06/16
 * @brief
 * @attention  注意事项--
 * @ChangeLogs
 *  Date             Author          Notes
 *  2026/06/16       ouqingfeng          First version
 ********************************************************************************/
//---------------------------------文件包含--------------------------------------
#include "LED.h"
//---------------------------------本文内宏变量定义--------------------------------
//---------------------------------本文内宏命令定义--------------------------------
//---------------------------------本文内数据结构定义------------------------------
//---------------------------------本文件内变量定义-------------------------------
LED_S led1 = {
    .mode = LED_MODE_OFF,
    .io.Port = NULL, // ESP32不使用端口寄存器，保持NULL
    .io.Pin = 48,    // GPIO48连接LED1
    .io.Polar = GPIO_POLAR_INVERT};
//---------------------------------本文件内函数声明-------------------------------
static void recalc_timing(LED_S *pLed, float freq_hz, float duty_cycle);
//---------------------------------本文件内函数定义-------------------------------
// 内部函数：根据频率和占空比重计算定时参数
static void recalc_timing(LED_S *pLed, float freq_hz, float duty_cycle)
{
    // 限制频率有效范围，防止除零或超高频率
    if (freq_hz < 0.1f)
    {
        freq_hz = 0.1f;
    }

    if (freq_hz > 24.0f) // 人眼可观察的最大频率
    {
        freq_hz = 24.0f;
    }

    // 限制占空比
    if (duty_cycle < 0.0f)
    {
        duty_cycle = 0.0f;
    }

    if (duty_cycle > 100.0f)
    {
        duty_cycle = 100.0f;
    }
    uint16_t period = (uint16_t)(1000.0f / freq_hz);
    // 保证周期至少 1ms，防止产生 0 周期导致死循环
    if (period < 1)
    {
        period = 1;
    }

    pLed->period_ms = period;

    pLed->blink_on_time = (uint16_t)(period * duty_cycle / 100.0f);
    pLed->blink_off_time = period - pLed->blink_on_time;

    // 极端情况：占空比 100% 或 0% 时，对应时间为 0，但不影响状态机安全
}

void LED_Init(LED_S *pLed, float freq_hz, float duty_cycle)
{
    if (pLed == NULL)
    {
        return;
    }

    if (pLed->mode == LED_MODE_BLINK)
    {
        recalc_timing(pLed, freq_hz, duty_cycle);
    }

    HAL_GPIO_CONFIG_S iocfg = {
        .Mode = HAL_GPIO_MODE_OUTPUT_PP,
        .Pull = HAL_GPIO_PULL_NONE,
    };

    HAL_GpioInit(&pLed->io, &iocfg);
    HAL_GpioWrite(&pLed->io, LED_STATE_OFF);
}

void LED_SetMode(LED_S *pLed, LED_MODE_E mode)
{
    pLed->mode = mode;
}
void LED_SetState(LED_S *pLed, LED_STATE_E state)
{
    HAL_GpioWrite(&pLed->io, state);
}

void LED_SetBlink(LED_S *pLed, float freq_hz, float duty_cycle)
{
    if (pLed->mode == LED_MODE_BLINK)
    {
        recalc_timing(pLed, freq_hz, duty_cycle);
        pLed->last_time = 0;
        LED_SetState(pLed, LED_STATE_ON); // 从亮开始
    }
}

void LED_Task(LED_S *pLed, uint64_t current_time)
{
    uint64_t elapsed = 0;
    uint16_t target = 0;

    pLed->State = HAL_GpioRead(&pLed->io); // 更新led状态

    switch (pLed->mode)
    {
    case LED_MODE_OFF: // 常灭
        LED_SetState(pLed, LED_STATE_OFF);
        break;

    case LED_MODE_ON: // 常亮
        LED_SetState(pLed, LED_STATE_ON);
        break;

    case LED_MODE_BLINK: // 闪烁
    {
        // 首次进入或刚切换模式，初始化时间基准
        if (pLed->last_time == 0)
        {
            pLed->last_time = current_time;
            return;
        }

        // 计算时间差，保留为 64 位避免溢出
        elapsed = current_time - pLed->last_time;
        target = (pLed->State == LED_STATE_ON ? pLed->blink_on_time : pLed->blink_off_time);

        if (pLed->blink_on_time == 0)
        {
            LED_SetState(pLed, LED_STATE_OFF);
        }
        else if ((pLed->blink_off_time == 0))
        {
            LED_SetState(pLed, LED_STATE_ON);
        }
        else
        {
            if (elapsed < target)
            {
                return; // 时间未到，不处理
            }
            // 执行状态翻转
            pLed->last_time = current_time;
            LED_SetState(pLed, !pLed->State);
        }
    }
    default:
        break;
    }
}
