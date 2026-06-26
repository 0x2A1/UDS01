/********************************************************************************
 * @copyright   Copyright (c)
 * @file        debug.h
 * @author      ouqingfeng
 * @date        2026/06/22
 * @brief
 * @attention  注意事项--
 * @ChangeLogs
 *  Date             Author          Notes
 *  2026/06/22       ouqingfeng          First version
 ********************************************************************************/
#ifndef _DEBUG_H_
#define _DEBUG_H_
//---------------------------------文件包含---------------------------------------
#include "esp_log.h"
//---------------------------------本文内宏变量定义--------------------------------
//---------------------------------本文内宏命令定义--------------------------------
#define DEBUG_HEX(...) esp_log_buffer_hex("HEX", __VA_ARGS__)
#define DEBUG_W(...) ESP_LOGW("Warning", __VA_ARGS__) // 黄色
#define DEBUG_E(...) ESP_LOGE("Error", __VA_ARGS__)   // 红色
#define DEBUG_I(...) ESP_LOGI("Info", __VA_ARGS__)    // 绿色
// 白色 如果不显示可以在menuconfig中 Component config → Log output → Default log verbosity 中调整默认日志级别为DEBUG
#define DEBUG_D(...)                                    \
    {                                                   \
        ESP_LOGW("DEBUG", __VA_ARGS__);                 \
        ESP_LOGW("DEBUG", "%s:%d", __FILE__, __LINE__); \
    }
#define DEBUG_D_HEX(...)                                \
    {                                                   \
        ESP_LOGW("DEBUG", "DEBUG HEX:");                \
        esp_log_buffer_hex("DEBUG", __VA_ARGS__);       \
        ESP_LOGW("DEBUG", "%s:%d", __FILE__, __LINE__); \
    }
//---------------------------------本文内数据结构定义------------------------------
//---------------------------------本文件内变量声明--------------------------------
//---------------------------------本文件内函数声明--------------------------------
#endif /*DEBUG_H_*/
