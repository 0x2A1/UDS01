/********************************************************************************
 * @copyright   Copyright (c)
 * @file        bsp_uart.c
 * @author      ouqingfeng
 * @date        2026/06/26
 * @brief       ESP32 平台串口硬件抽象层实现
 * @attention   - 基于 ESP-IDF UART 驱动，使用静态内存池管理实例
 *              - 最多支持 UART_MAX_NUM 个串口（ESP32-S3 最多3个）
 *              - 实际占用硬件 UART 编号由驱动自动分配（默认按调用顺序）
 * @ChangeLogs
 *  Date             Author          Notes
 *  2026/06/26       ouqingfeng      First version
 ********************************************************************************/
#include "bsp_config.h"
#include "driver/uart.h"
#include "hal/uart_types.h"
#include "../../HAL/include/hal_uart.h"

#define TAG "BSP_UART"

//---------------------------------内部宏----------------------------------------
#define UART_HW_FIFO_LEN (128) // 硬件 FIFO 大小（ESP32 固定 128 字节）

//---------------------------------数据结构定义-----------------------------------
struct UART_S
{
    uart_port_t port; ///< ESP32 硬件 UART 端口号
    bool is_used;     ///< 槽位占用标志
    bool initialized; ///< 是否已完成初始化
};

//---------------------------------静态池----------------------------------------
static UART_S gUartPool[UART_MAX_NUM];
static uart_port_t g_next_port = UART_NUM_0; // 自动分配端口号

//---------------------------------内部函数---------------------------------------
/**
 * @brief 查找空闲槽位
 */
static UART_S *find_free_uart(void)
{
    for (int i = 0; i < UART_MAX_NUM; i++)
    {
        if (!gUartPool[i].is_used)
        {
            return &gUartPool[i];
        }
    }
    return NULL;
}

/**
 * @brief 将本模块的数据结构枚举转换为 ESP-IDF 的常量
 */
static uart_word_length_t map_data_bits(UART_DATA_BITS_E bits)
{
    switch (bits)
    {
    case UART_DATA_5_BITS:
        return UART_DATA_5_BITS;
    case UART_DATA_6_BITS:
        return UART_DATA_6_BITS;
    case UART_DATA_7_BITS:
        return UART_DATA_7_BITS;
    case UART_DATA_8_BITS:
    default:
        return UART_DATA_8_BITS;
    }
}

static uart_stop_bits_t map_stop_bits(UART_STOP_BITS_E bits)
{
    switch (bits)
    {
    case UART_STOP_BITS_1_5:
        return UART_STOP_BITS_1_5;
    case UART_STOP_BITS_2:
        return UART_STOP_BITS_2;
    case UART_STOP_BITS_1:
    default:
        return UART_STOP_BITS_1;
    }
}

static uart_parity_t map_parity(UART_PARITY_E parity)
{
    switch (parity)
    {
    case UART_PARITY_EVEN:
        return UART_PARITY_EVEN;
    case UART_PARITY_ODD:
        return UART_PARITY_ODD;
    case UART_PARITY_DISABLE:
    default:
        return UART_PARITY_DISABLE;
    }
}

static uart_hw_flowcontrol_t map_flow_ctrl(UART_HW_FLOWCTRL_E flow)
{
    switch (flow)
    {
    case UART_HW_FLOWCTRL_RTS:
        return UART_HW_FLOWCTRL_RTS;
    case UART_HW_FLOWCTRL_CTS:
        return UART_HW_FLOWCTRL_CTS;
    case UART_HW_FLOWCTRL_CTS_RTS:
        return UART_HW_FLOWCTRL_CTS_RTS;
    case UART_HW_FLOWCTRL_DISABLE:
    default:
        return UART_HW_FLOWCTRL_DISABLE;
    }
}

//---------------------------------对外接口实现-----------------------------------

UART_S *uart_alloc(void)
{
    UART_S *pUart = find_free_uart();
    if (pUart)
    {
        memset(pUart, 0, sizeof(UART_S));
        pUart->is_used = true;
        ESP_LOGI(TAG, "UART allocated at slot %d", (int)(pUart - gUartPool));
    }
    else
    {
        ESP_LOGE(TAG, "No free UART slot");
    }
    return pUart;
}

void uart_free(UART_S *pUart)
{
    if (pUart && pUart->is_used)
    {
        pUart->is_used = false;
    }
}

int8_t uart_init(UART_S *pUart, UART_CONFIG_S config)
{
    assert(pUart != NULL);
    if (!pUart || !pUart->is_used)
    {
        ESP_LOGE(TAG, "Invalid UART pointer");
        return -1;
    }

    // 分配一个硬件端口（简单的轮转分配，可扩展更复杂的策略）
    uart_port_t port = g_next_port;
    if (port > UART_NUM_2)
    {
        ESP_LOGE(TAG, "No hardware UART available");
        return -1;
    }
    g_next_port++;

    // 1. 配置参数
    uart_config_t uart_config = {
        .baud_rate = config.baud_rate,
        .data_bits = map_data_bits(config.data_bits),
        .parity = map_parity(config.parity),
        .stop_bits = map_stop_bits(config.stop_bits),
        .flow_ctrl = map_flow_ctrl(config.flow_ctrl),
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t ret = uart_param_config(port, &uart_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Parameter config failed: %s", esp_err_to_name(ret));
        return -1;
    }

    // 2. 设置引脚（-1 表示不设置）
    ret = uart_set_pin(port, config.tx_io, config.rx_io,
                       config.rts_io, config.cts_io);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Set pin failed: %s", esp_err_to_name(ret));
        return -1;
    }

    // 3. 安装驱动程序（分配队列/缓冲区）
    int tx_buf = (config.tx_buf_size > 0) ? config.tx_buf_size : 256;
    int rx_buf = (config.rx_buf_size > 0) ? config.rx_buf_size : 256;
    ret = uart_driver_install(port, rx_buf, tx_buf, 0, NULL, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Driver install failed: %s", esp_err_to_name(ret));
        return -1;
    }

    // 4. 保存状态
    pUart->port = port;
    pUart->initialized = true;

    ESP_LOGI(TAG, "UART init OK, port=%d, baud=%lu", port, (unsigned long)config.baud_rate);
    return 0;
}

int uart_send(UART_S *pUart, const uint8_t *data, uint32_t len)
{
    assert(pUart != NULL);
    if (!pUart || !pUart->initialized || !data || len == 0)
    {
        return -1;
    }
    int sent = uart_write_bytes(pUart->port, data, len);
    return sent;
}

int uart_receive(UART_S *pUart, uint8_t *buf, uint32_t max_len, uint32_t timeout_ms)
{
    assert(pUart != NULL);
    if (!pUart || !pUart->initialized || !buf || max_len == 0)
    {
        return -1;
    }
    if (timeout_ms == 0)
    {
        // 非阻塞
        uint32_t avail = 0;
        uart_get_buffered_data_len(pUart->port, (size_t *)&avail);
        if (avail == 0)
            return 0;
        uint32_t read_len = (avail < max_len) ? avail : max_len;
        int len = uart_read_bytes(pUart->port, buf, read_len, 0);
        return len;
    }
    else
    {
        // 带超时
        TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
        int len = uart_read_bytes(pUart->port, buf, max_len, ticks);
        return len;
    }
}

uint32_t uart_get_rx_available(UART_S *pUart)
{
    if (!pUart || !pUart->initialized)
        return 0;
    size_t avail = 0;
    uart_get_buffered_data_len(pUart->port, &avail);
    return (uint32_t)avail;
}

void uart_deinit(UART_S *pUart)
{
    assert(pUart != NULL);
    if (!pUart || !pUart->initialized)
        return;

    uart_driver_delete(pUart->port);
    pUart->port = UART_NUM_MAX;
    pUart->initialized = false;
    pUart->is_used = false;
    ESP_LOGI(TAG, "UART deinitialized");
}