#include "Global.h"
#include "debug.h"
#include "../Drivers/LED/LED.h"
#include "../HAL/include/hal_timer.h"
#include "rtosTask.h"
unsigned char DataStr[] = __DATE__;
unsigned char TimeStr[] = __TIME__;

void app_main(void)
{
    GlobalInit();

    LED_Init(&led1, 1.0f, 50); // 初始化LED1，频率1Hz，占空比50%
    TaskInit();
    while (1)
    {
    }
}
