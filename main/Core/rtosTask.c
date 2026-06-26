
/*********************************************
 * @copyright   Copyright (c)
 * @file        rtosTask.c
 * @author      ouqingfeng
 * @date        2026/06/05
 * @brief
 * @attention  注意事项--
 * @ChangeLogs
 *  Date             Author          Notes
 *  2026/06/05       ouqingfeng          First version
 **********************************************/
//---------------------------------文件包含------------------------------------------------------
#include "rtosTask.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <freertos/semphr.h>
#include "freertos/event_groups.h"

#include "../Drivers/LED/LED.h"
//---------------------------------本文件内宏定义------------------------------------------------
//---------------------------------本文件内函数声明----------------------------------------------
//---------------------------------本文件内变量定义----------------------------------------------
TaskHandle_t Led_Handle;
// //---------------------------------本文件内函数定义-------------------------------------------

void Led_Task(void *agv)
{
    static uint8_t i = 0;
    for (;;)
    {
        LED_SetState(&led1, i);
        i = !i;
        vTaskDelay((100 / portTICK_PERIOD_MS));
    }
}

void TaskInit(void)
{
    xTaskCreate(&Led_Task, "Led_Task", 1024 * 3, NULL, 1, &Led_Handle);
}
