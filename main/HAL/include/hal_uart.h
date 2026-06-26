/********************************************************************************
 * @copyright   Copyright (c)
 * @file        hal_uart.h
 * @author      ouqingfeng
 * @date        2026/06/26
 * @brief       硬件抽象层 - 通用串口接口
 * @attention   本接口基于 ESP32 UART 驱动实现，向上提供统一的操作函数。
 *              最多支持 UART_MAX_NUM 个串口实例。
 * @ChangeLogs
 *  Date             Author          Notes
 *  2026/06/26       ouqingfeng      First version
 ********************************************************************************/
#ifndef _HAL_UART_H_
#define _HAL_UART_H_

#include "../../Core/Global.h" //

//---------------------------------宏定义---------------------------------------
#define UART_MAX_NUM 3 ///< 最多支持的串口数量（ESP32-S3 有3个硬件UART）

//---------------------------------数据结构--------------------------------------
/* 不透明结构体：实现细节隐藏在 bsp_uart.c 中 */
struct UART_S;
typedef struct UART_S UART_S;

/**
 * @brief 数据位长度
 */
typedef enum
{
    UART_DATA_5_BITS = 0,
    UART_DATA_6_BITS,
    UART_DATA_7_BITS,
    UART_DATA_8_BITS,
    UART_DATA_BITS_MAX,
} UART_DATA_BITS_E;

/**
 * @brief 停止位
 */
typedef enum
{
    UART_STOP_BITS_1 = 0,
    UART_STOP_BITS_1_5,
    UART_STOP_BITS_2,
    UART_STOP_BITS_MAX,
} UART_STOP_BITS_E;

/**
 * @brief 校验位
 */
typedef enum
{
    UART_PARITY_DISABLE = 0,
    UART_PARITY_EVEN,
    UART_PARITY_ODD,
    UART_PARITY_MAX,
} UART_PARITY_E;

/**
 * @brief 硬件流控
 */
typedef enum
{
    UART_HW_FLOWCTRL_DISABLE = 0,
    UART_HW_FLOWCTRL_RTS,
    UART_HW_FLOWCTRL_CTS,
    UART_HW_FLOWCTRL_CTS_RTS,
    UART_HW_FLOWCTRL_MAX,
} UART_HW_FLOWCTRL_E;

/**
 * @brief UART 配置结构体
 */
typedef struct
{
    uint32_t baud_rate;           ///< 波特率，如 115200
    UART_DATA_BITS_E data_bits;   ///< 数据位
    UART_STOP_BITS_E stop_bits;   ///< 停止位
    UART_PARITY_E parity;         ///< 校验位
    UART_HW_FLOWCTRL_E flow_ctrl; ///< 硬件流控模式
    int8_t tx_io;                 ///< TX 引脚号（-1 表示不启用）
    int8_t rx_io;                 ///< RX 引脚号（-1 表示不启用）
    int8_t rts_io;                ///< RTS 引脚号（-1 表示不使用）
    int8_t cts_io;                ///< CTS 引脚号（-1 表示不使用）
    uint32_t rx_buf_size;         ///< 接收环形缓冲区大小（字节）
    uint32_t tx_buf_size;         ///< 发送环形缓冲区大小（字节）
} UART_CONFIG_S;

//---------------------------------函数声明--------------------------------------

/**
 * @brief 从静态池中分配一个 UART 实例
 * @return 指向 UART_S 的指针，无空闲时返回 NULL
 */
UART_S *uart_alloc(void);

/**
 * @brief 归还 UART 实例到静态池（应在 deinit 之后调用）
 * @param pUart 待释放的实例指针
 */
void uart_free(UART_S *pUart);

/**
 * @brief 初始化 UART 硬件并配置参数（不占用引脚？默认占用）
 * @param pUart   已分配的 UART 实例
 * @param config  串口配置
 * @return 0 成功，-1 失败
 * @note  初始化后可使用读写函数，无需再调用 start
 */
int8_t uart_init(UART_S *pUart, UART_CONFIG_S config);

/**
 * @brief 发送数据
 * @param pUart   UART 实例
 * @param data    数据缓冲区
 * @param len     发送长度
 * @return 实际发送的字节数，<0 表示错误
 */
int uart_send(UART_S *pUart, const uint8_t *data, uint32_t len);

/**
 * @brief 接收数据（非阻塞）
 * @param pUart   UART 实例
 * @param buf     接收缓冲区
 * @param max_len 期望最大接收长度
 * @param timeout_ms 超时时间（毫秒），0 表示非阻塞立即返回
 * @return 实际读取的字节数，<0 表示错误
 */
int uart_receive(UART_S *pUart, uint8_t *buf, uint32_t max_len, uint32_t timeout_ms);

/**
 * @brief 查询接收缓冲区中可读的字节数
 * @param pUart UART 实例
 * @return 可读字节数
 */
uint32_t uart_get_rx_available(UART_S *pUart);

/**
 * @brief 反初始化 UART，释放硬件资源并标记槽位空闲
 * @param pUart UART 实例
 */
void uart_deinit(UART_S *pUart);

#endif /*_HAL_UART_H_*/