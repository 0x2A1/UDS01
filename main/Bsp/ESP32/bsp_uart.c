/********************************************************************************
 * @file        bsp_uart.c
 * @author      ouqingfeng
 * @date        2026/06/29
 * @brief       ESP32 UART HAL 实现（使用空闲索引栈管理实例和端口）
 * @attention   - 使用静态池 + 空闲索引栈，分配/释放 O(1)
 *              - 硬件端口动态分配，支持释放后复用
 *              - 需要先 uart_alloc()，再 uart_init()，最后 uart_deinit() + uart_free()
 ********************************************************************************/
#include "bsp_config.h"
#include "driver/uart.h"
#include "hal/uart_types.h"
#include "../../HAL/include/hal_uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "BSP_UART"

//---------------------------------静态断言--------------------------------------
// 确保软件实例数不超过硬件支持的最大端口数
_Static_assert(UART_MAX_NUM <= UART_NUM_MAX, "UART_MAX_NUM exceeds hardware UART count");

//---------------------------------数据结构定义-----------------------------------
struct UART_S
{
    uart_port_t port; // 硬件端口号，UART_NUM_MAX 表示未分配
    bool initialized; // 硬件是否已初始化（驱动已安装）
    bool in_use;      // 实例是否已被分配（由池管理）
};

//---------------------------------静态池与栈-----------------------------------
static UART_S g_uart_pool[UART_MAX_NUM];

// 空闲实例索引栈（存储可用槽位下标）
static uint16_t g_free_inst_stack[UART_MAX_NUM];
static uint16_t g_inst_top; // 栈顶指针，指向下一个空闲位置

// 空闲硬件端口栈（存储可用端口号，值为 uart_port_t 的整数）
static uint16_t g_free_port_stack[UART_MAX_NUM];
static uint16_t g_port_top;

// 模块初始化标志
static bool g_module_initialized = false;

//---------------------------------内部函数---------------------------------------
/**
 * @brief 初始化实例池和端口栈（只执行一次）
 */
static void init_pools(void)
{
    if (g_module_initialized)
    {
        return;
    }

    // 初始化实例栈：填入 0 ~ UART_MAX_NUM-1
    for (int i = 0; i < UART_MAX_NUM; i++)
    {
        g_free_inst_stack[i] = i;
    }
    g_inst_top = UART_MAX_NUM;

    // 初始化端口栈：填入 0 ~ UART_MAX_NUM-1 (对应 UART_NUM_0,1,...)
    for (int i = 0; i < UART_MAX_NUM; i++)
    {
        g_free_port_stack[i] = i;
    }
    g_port_top = UART_MAX_NUM;

    // 清空池状态
    memset(g_uart_pool, 0, sizeof(g_uart_pool));

    g_module_initialized = true;
    ESP_LOGI(TAG, "Pools initialized (instances=%d, ports=%d)", UART_MAX_NUM, UART_MAX_NUM);
}

/**
 * @brief 从栈中弹出一个索引（或端口号）
 * @param stack 栈数组
 * @param top   栈顶指针（传入传出）
 * @param size  栈最大容量
 * @return 弹出的值，若栈空则返回 -1
 */
static int16_t stack_pop(uint16_t *stack, uint16_t *top, uint16_t size)
{
    if (*top == 0)
    {
        return -1;
    }
    (*top)--;
    return stack[*top];
}

/**
 * @brief 将值压入栈
 * @param stack 栈数组
 * @param top   栈顶指针（传入传出）
 * @param size  栈最大容量
 * @param val   要压入的值
 * @return true 成功，false 栈满
 */
static bool stack_push(uint16_t *stack, uint16_t *top, uint16_t size, uint16_t val)
{
    if (*top >= size)
    {
        return false;
    }
    stack[*top] = val;
    (*top)++;
    return true;
}

//---------------------------------映射函数（保持不变）-----------------------------
static uart_word_length_t map_data_bits(UART_DATA_BITS_E bits) { /* ... */ }
static uart_stop_bits_t map_stop_bits(UART_STOP_BITS_E bits) { /* ... */ }
static uart_parity_t map_parity(UART_PARITY_E parity) { /* ... */ }
static uart_hw_flowcontrol_t map_flow_ctrl(UART_HW_FLOWCTRL_E flow) { /* ... */ }
// 为节省篇幅，此处省略具体实现（与原代码相同）

//---------------------------------对外接口实现-----------------------------------

UART_S *uart_alloc(void)
{
    init_pools(); // 确保池已初始化

    // 临界区保护（可选，防止多任务并发）
    portENTER_CRITICAL(&g_uart_spinlock);
    int16_t idx = stack_pop(g_free_inst_stack, &g_inst_top, UART_MAX_NUM);
    portEXIT_CRITICAL(&g_uart_spinlock);

    if (idx < 0)
    {
        ESP_LOGE(TAG, "No free UART instance");
        return NULL;
    }

    UART_S *pUart = &g_uart_pool[idx];
    // 重置结构体（保留必要字段）
    pUart->port = UART_NUM_MAX; // 无效端口
    pUart->initialized = false;
    pUart->in_use = true;

    ESP_LOGI(TAG, "UART allocated at slot %d", idx);
    return pUart;
}

void uart_free(UART_S *pUart)
{
    if (!pUart)
    {
        ESP_LOGE(TAG, "uart_free: NULL pointer");
        return;
    }

    // 检查指针是否在池范围内
    int idx = pUart - g_uart_pool;
    if (idx < 0 || idx >= UART_MAX_NUM)
    {
        ESP_LOGE(TAG, "uart_free: invalid pointer (out of pool)");
        return;
    }

    // 检查实例是否已被分配
    if (!pUart->in_use)
    {
        ESP_LOGW(TAG, "uart_free: instance %d not in use", idx);
        return;
    }

    // 检查是否仍处于初始化状态（必须先 deinit）
    if (pUart->initialized)
    {
        ESP_LOGE(TAG, "uart_free: instance %d still initialized, call uart_deinit first", idx);
        return;
    }

    // 归还实例索引到空闲栈
    portENTER_CRITICAL(&g_uart_spinlock);
    bool ok = stack_push(g_free_inst_stack, &g_inst_top, UART_MAX_NUM, (uint16_t)idx);
    portEXIT_CRITICAL(&g_uart_spinlock);

    if (ok)
    {
        pUart->in_use = false; // 标记未使用
        ESP_LOGI(TAG, "UART instance %d freed", idx);
    }
    else
    {
        ESP_LOGE(TAG, "uart_free: stack full, internal error");
    }
}

int8_t uart_init(UART_S *pUart, UART_CONFIG_S config)
{
    if (!pUart)
    {
        ESP_LOGE(TAG, "uart_init: NULL pointer");
        return -1;
    }

    int idx = pUart - g_uart_pool;
    if (idx < 0 || idx >= UART_MAX_NUM || !pUart->in_use)
    {
        ESP_LOGE(TAG, "uart_init: invalid or unallocated instance");
        return -1;
    }

    if (pUart->initialized)
    {
        ESP_LOGW(TAG, "uart_init: instance already initialized");
        return 0; // 可视为成功
    }

    // 从端口栈分配一个硬件端口
    portENTER_CRITICAL(&g_uart_spinlock);
    int16_t port = stack_pop(g_free_port_stack, &g_port_top, UART_MAX_NUM);
    portEXIT_CRITICAL(&g_uart_spinlock);

    if (port < 0)
    {
        ESP_LOGE(TAG, "No free hardware UART port");
        return -1;
    }

    uart_port_t uart_port = (uart_port_t)port;
    ESP_LOGI(TAG, "Assigning hardware port %d to instance %d", uart_port, idx);

    // 配置参数
    uart_config_t uart_config = {
        .baud_rate = config.baud_rate,
        .data_bits = map_data_bits(config.data_bits),
        .parity = map_parity(config.parity),
        .stop_bits = map_stop_bits(config.stop_bits),
        .flow_ctrl = map_flow_ctrl(config.flow_ctrl),
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t ret = uart_param_config(uart_port, &uart_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        goto err_release_port;
    }

    // 设置引脚
    ret = uart_set_pin(uart_port, config.tx_io, config.rx_io,
                       config.rts_io, config.cts_io);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(ret));
        goto err_release_port;
    }

    // 安装驱动
    int tx_buf = (config.tx_buf_size > 0) ? config.tx_buf_size : 256;
    int rx_buf = (config.rx_buf_size > 0) ? config.rx_buf_size : 256;
    ret = uart_driver_install(uart_port, rx_buf, tx_buf, 0, NULL, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        goto err_release_port;
    }

    // 保存状态
    pUart->port = uart_port;
    pUart->initialized = true;

    ESP_LOGI(TAG, "UART init OK, port=%d, baud=%lu", uart_port, (unsigned long)config.baud_rate);
    return 0;

err_release_port:
    // 归还端口
    portENTER_CRITICAL(&g_uart_spinlock);
    stack_push(g_free_port_stack, &g_port_top, UART_MAX_NUM, (uint16_t)port);
    portEXIT_CRITICAL(&g_uart_spinlock);
    return -1;
}

void uart_deinit(UART_S *pUart)
{
    if (!pUart)
    {
        ESP_LOGE(TAG, "uart_deinit: NULL pointer");
        return;
    }

    int idx = pUart - g_uart_pool;
    if (idx < 0 || idx >= UART_MAX_NUM || !pUart->in_use)
    {
        ESP_LOGE(TAG, "uart_deinit: invalid or unallocated instance");
        return;
    }

    if (!pUart->initialized)
    {
        ESP_LOGW(TAG, "uart_deinit: instance %d not initialized", idx);
        return;
    }

    uart_port_t port = pUart->port;
    if (port >= UART_NUM_MAX)
    {
        ESP_LOGE(TAG, "uart_deinit: invalid port number");
        return;
    }

    // 卸载驱动
    esp_err_t ret = uart_driver_delete(port);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_driver_delete failed: %s", esp_err_to_name(ret));
    }

    // 归还端口到空闲栈
    portENTER_CRITICAL(&g_uart_spinlock);
    bool ok = stack_push(g_free_port_stack, &g_port_top, UART_MAX_NUM, (uint16_t)port);
    portEXIT_CRITICAL(&g_uart_spinlock);

    if (!ok)
    {
        ESP_LOGE(TAG, "uart_deinit: port stack full, internal error");
    }

    pUart->port = UART_NUM_MAX;
    pUart->initialized = false;
    ESP_LOGI(TAG, "UART deinitialized (port %d freed)", port);
}

// uart_send, uart_receive, uart_get_rx_available 与原代码基本相同，仅加入更严格的校验
int uart_send(UART_S *pUart, const uint8_t *data, uint32_t len)
{
    if (!pUart || !pUart->in_use || !pUart->initialized || !data || len == 0)
    {
        return -1;
    }
    int sent = uart_write_bytes(pUart->port, (const char *)data, len);
    return (sent < 0) ? -1 : sent;
}

int uart_receive(UART_S *pUart, uint8_t *buf, uint32_t max_len, uint32_t timeout_ms)
{
    if (!pUart || !pUart->in_use || !pUart->initialized || !buf || max_len == 0)
    {
        return -1;
    }
    TickType_t ticks = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);
    int len = uart_read_bytes(pUart->port, buf, max_len, ticks);
    return (len < 0) ? -1 : len;
}

uint32_t uart_get_rx_available(UART_S *pUart)
{
    if (!pUart || !pUart->in_use || !pUart->initialized)
    {
        return 0;
    }
    size_t avail = 0;
    uart_get_buffered_data_len(pUart->port, &avail);
    return (uint32_t)avail;
}