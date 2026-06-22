/********************************************************************************
 * @copyright   Copyright (c)
 * @file        bsp_gpio.c
 * @author      ouqingfeng
 * @date        2026/06/05
 * @brief
 * @attention  注意事项--
 * @ChangeLogs
 *  Date             Author          Notes
 *  2026/06/05       ouqingfeng          First version
 ********************************************************************************/
//---------------------------------文件包含------------------------------------------------------
#include "driver/gpio.h"
#include "../../../HAL/include/hal_gpio.h"
//---------------------------------本文件内宏定义------------------------------------------------
//---------------------------------本文件内变量定义----------------------------------------------
//---------------------------------本文件内函数声明----------------------------------------------
//---------------------------------本文件内函数定义----------------------------------------------
/**
 * @brief  GPIO引脚始化
 * @param  pGpio: 待操作GPIO引脚指针
 * @param  cfg: GPIO配置参数指针
 * @retval 无
 */
void HAL_GpioInit(const HAL_GPIO_S *pGpio, const HAL_GPIO_CONFIG_S *cfg)
{
    if (pGpio == NULL)
    {
        return;
    }
    gpio_config_t io_conf = {0};
    io_conf.pin_bit_mask = (1ULL << pGpio->Pin);

    switch (cfg->Mode)
    {
    case HAL_GPIO_MODE_INPUT:
        io_conf.mode = GPIO_MODE_INPUT;
        break;
    case HAL_GPIO_MODE_OUTPUT_PP:
        io_conf.mode = GPIO_MODE_OUTPUT;
        break;
    case HAL_GPIO_MODE_OUTPUT_OP:
        io_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
        break;
    case HAL_GPIO_MODE_ALT_FUNCTION:
        /* code */
        break;
    case HAL_GPIO_MODE_ANALOG:
        /* code */
        break;
    default:
        io_conf.mode = GPIO_MODE_DISABLE;
        break;
    }

    switch (cfg->Pull)
    {
    case HAL_GPIO_PULL_NONE:
        io_conf.pull_up_en = 0;
        io_conf.pull_down_en = 0;
        break;

    case HAL_GPIO_PULL_UP:
        io_conf.pull_up_en = 1;
        io_conf.pull_down_en = 0;
        break;

    case HAL_GPIO_PULL_DOWN:
        io_conf.pull_up_en = 0;
        io_conf.pull_down_en = 1;
        break;

    default:
        io_conf.pull_up_en = 0;
        io_conf.pull_down_en = 0;
        break;
    }

    gpio_config(&io_conf);

    if (cfg->IrqCallback)
    {
        switch (cfg->IrqTrigger)
        {
        case GPIO_IRQ_TRIGGER_NONE:
            io_conf.intr_type = GPIO_INTR_DISABLE;
            break;

        case GPIO_IRQ_TRIGGER_RISING:
            io_conf.intr_type = GPIO_INTR_POSEDGE;
            break;

        case GPIO_IRQ_TRIGGER_FALLING:
            io_conf.intr_type = GPIO_INTR_NEGEDGE;

            break;

        case GPIO_IRQ_TRIGGER_BOTH:
            io_conf.intr_type = GPIO_INTR_ANYEDGE;
            break;

        default:

            break;
        }
        gpio_install_isr_service(0);
        gpio_isr_handler_add(pGpio->Pin, (gpio_isr_t)cfg->IrqCallback, cfg->IrqArg);
    }
}

/**
 * @brief  GPIO引脚反初始化，恢复默认状态
 * @param  pGpio: 待操作GPIO引脚指针
 * @retval 无
 */
void HAL_GpioDeinit(const HAL_GPIO_S *pGpio)
{
    if (pGpio == NULL)
    {
        return;
    }
    /* 移除该引脚的中断服务函数（如果已注册） */
    gpio_isr_handler_remove(pGpio->Pin);
    /* 将引脚复位为上电默认状态 */
    gpio_reset_pin(pGpio->Pin);
}

/**
 * @brief  读取GPIO引脚电平状态
 * @param  pGpio: 待读取GPIO引脚指针
 * @retval 0:低电平  1:高电平
 */
uint8_t HAL_GpioRead(const HAL_GPIO_S *pGpio)
{
    if (pGpio == NULL)
    {
        return 0;
    }
    return (uint8_t)gpio_get_level(pGpio->Pin) ^ pGpio->Polar;
}

/**
 * @brief  设置GPIO引脚输出电平
 * @param  pGpio: 待操作GPIO引脚指针
 * @param  value: 0-低电平  1-高电平
 * @retval 无
 */
void HAL_GpioWrite(const HAL_GPIO_S *pGpio, uint8_t value)
{
    if (pGpio == NULL)
    {
        return;
    }
    value ^= pGpio->Polar;
    gpio_set_level(pGpio->Pin, value);
}

/**
 * @brief  GPIO引脚电平翻转
 * @param  pGpio: 待翻转GPIO引脚指针
 * @retval 无
 */
void HAL_GpioToggle(const HAL_GPIO_S *pGpio)
{
    if (pGpio == NULL)
    {
        return;
    }
    /* 读取当前电平并写入相反值 */
    uint8_t value = HAL_GpioRead(pGpio);
    HAL_GpioWrite(pGpio, value);
}

/**
 * @brief  使能GPIO外部中断
 * @param  pGpio: 目标GPIO引脚指针
 * @retval 0:失败  1:成功
 */
uint8_t HAL_GpioIrqEnable(const HAL_GPIO_S *pGpio)
{
    if (pGpio == NULL)
    {
        return 0;
    }
    esp_err_t err = gpio_intr_enable(pGpio->Pin);
    return (err == ESP_OK) ? 1 : 0;
}

/**
 * @brief  关闭GPIO外部中断
 * @param  pGpio: 目标GPIO引脚指针
 * @retval 无
 */
void HAL_GpioIrqDisable(const HAL_GPIO_S *pGpio)
{
    if (pGpio == NULL)
    {
        return;
    }
    gpio_intr_disable(pGpio->Pin);
}