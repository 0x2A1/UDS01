/********************************************************************************
 * @copyright   Copyright (c)
 * @file        bsp_time.c
 * @author      ouqingfeng
 * @date        2026/06/23
 * @brief       ESP32 平台定时器硬件抽象层实现
 * @attention   - 基于 ESP-IDF GPTimer 驱动，使用静态内存池管理定时器实例
 *              - 分辨率固定为 1 MHz，对应分频系数为 80（满足硬件要求）
 *              - 最多支持 TIMER_MAX_NUM 个定时器
 *              - 中断回调已添加 IRAM_ATTR，确保 Flash 擦写期间仍能响应
 * @ChangeLogs
 *  Date             Author          Notes
 *  2026/06/23       ouqingfeng      First version
 *  2026/06/25       ouqingfeng      优化中断安全、生命周期管理、错误处理等
 ********************************************************************************/
//---------------------------------文件包含--------------------------------------
#include "bsp_config.h"
#include "driver/gptimer.h"
#include "hal/timer_types.h" // 提供 gptimer_alarm_count 等类型
#include "../../HAL/include/hal_timer.h"
//---------------------------------本文内宏变量定义--------------------------------
#define TAG "BSP_TIMER"
#define GPTIMER_RESOLUTION_HZ (1000000ULL)     // 定时器分辨率：1 MHz，分频系数 = 80
#define ALARM_COUNT_BASE GPTIMER_RESOLUTION_HZ // 报警计数值计算基准
#define TIMER_ISR_LOG_ENABLE 0                 // 中断回调日志开关：调试时置1，量产务必置0
//---------------------------------本文内宏命令定义--------------------------------
#if TIMER_ISR_LOG_ENABLE
#define TIMER_ISR_LOGI(...) ESP_EARLY_LOGI(TAG, __VA_ARGS__) // 中断安全日志
#else
#define TIMER_ISR_LOGI(...)
#endif
//---------------------------------本文内数据结构定义------------------------------
/* 定时器内部结构体 */
struct TIMER_S
{
    gptimer_handle_t handle; // 硬件句柄，由 gptimer_new_timer 创建
    bool is_used;            // 槽位占用标志（静态池管理）
    bool is_running;         // 硬件定时器是否已使能并启动
    TIMER_CALLBACK callback; // 用户注册的中断回调函数
    void *arg;               // 用户回调参数（当前未对外提供设置接口，预留）
    TIMER_CONFIG_S config;   // 配置备份，可用于重新初始化或调试
};
//---------------------------------本文件内变量定义-------------------------------
static TIMER_S gTimerPool[TIMER_MAX_NUM]; // 静态内存池：所有定时器实例均从此分配，无动态内存开销
//---------------------------------本文件内函数声明-------------------------------
//---------------------------------本文件内函数定义-------------------------------
/*********************************************************************************
 * @name          timer_isr_callback
 * @brief         定时器硬件中断回调（IRAM_ATTR 保证可执行性）
 * @note          回调函数运行在硬件中断上下文中，严禁使用带锁的函数（如 ESP_LOGx、printf），
 *                推荐仅操作 volatile 变量或调用 FreeRTOS FromISR API。
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         timer：GPTimer 句柄（未使用）
 * @param         edata：报警事件数据，包含当前计数值等信息
 * @param         user_ctx：指向 TIMER_S 实例的指针（注册时传入）
 * @return        返回值：false 表示没有高优先级任务被唤醒
 *********************************************************************************/
static bool IRAM_ATTR timer_isr_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    TIMER_S *pTimer = (TIMER_S *)user_ctx;
    // 有效性快速检查
    if (!pTimer || !pTimer->is_used)
    {
        return false;
    }

    // 执行用户回调（用户需保证回调函数是中断安全的）
    if (pTimer->callback)
    {
        pTimer->callback(pTimer->arg);
    }
    return false;
}
/*********************************************************************************
 * @name          find_free_timer
 * @brief         在静态池中查找第一个未使用的槽位
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         输入参数：代表含义
 * @return        返回值：空闲的 TIMER_S 指针，全满时返回 NULL
 *********************************************************************************/
static TIMER_S *find_free_timer(void)
{
    for (int i = 0; i < TIMER_MAX_NUM; i++)
    {
        if (!gTimerPool[i].is_used)
        {
            return &gTimerPool[i];
        }
    }
    return NULL;
}

/*********************************************************************************
 * @name          timer_alloc
 * @brief         从静态池分配一个定时器实例（仅标记槽位，不初始化硬件）
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：代表含义
 * @return         指向空闲 TIMER_S 的指针，若全满则返回 NULL
 *********************************************************************************/
TIMER_S *timer_alloc(void)
{
    TIMER_S *pTimer = find_free_timer();
    if (pTimer)
    {
        memset(pTimer, 0, sizeof(TIMER_S)); // 清零所有字段（is_used 会在此后置 true）
        pTimer->is_used = true;
        ESP_LOGI(TAG, "Timer allocated at slot %d", (int)(pTimer - gTimerPool));
    }
    else
    {
        ESP_LOGE(TAG, "No free timer slot");
    }
    return pTimer;
}

/*********************************************************************************
 * @name          timer_free
 * @brief         归还定时器槽位到静态池（应在 deinit 之后调用）
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         待释放的实例指针
 * @return        返回值：代表含义
 *********************************************************************************/

void timer_free(TIMER_S *pTimer)
{
    if (pTimer && pTimer->is_used)
    {
        pTimer->is_used = false;
    }
}
/*********************************************************************************
 * @name          timer_init
 * @brief         函数功能简述
 * @note          本函数不使能定时器，也不启动计数，需调用 timer_start 才能真正运行
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：由 timer_alloc() 获得的实例指针
 * @param         Config：定时器参数配置
 * @param         Callback：中断回调函数
 * @param         CallbackArg：中断回调函数传入的参数
 * @return        返回值：0 成功，-1 失败（参数无效或硬件操作错误）
 *********************************************************************************/
int8_t timer_init(TIMER_S *pTimer, TIMER_CONFIG_S Config, TIMER_CALLBACK Callback, void *CallbackArg)
{
    if (!pTimer || !pTimer->is_used)
    {
        ESP_LOGE(TAG, "Invalid timer pointer");
        return -1;
    }
    if (!Callback)
    {
        ESP_LOGE(TAG, "Callback is NULL");
        return -1;
    }

    // 报警计数值计算：ALARM_COUNT_BASE / freq_hz
    uint64_t alarm_count = ALARM_COUNT_BASE / Config.freq_hz;
    // 频率边界检查（频率不能过高导致 alarm_count < 2）
    if (Config.freq_hz == 0 || alarm_count < 2 || alarm_count > UINT32_MAX)
    {
        ESP_LOGE(TAG, "Invalid freq_hz=%lu (alarm_count=%llu out of range)",
                 (unsigned long)Config.freq_hz, (unsigned long long)alarm_count);
        return -1;
    }

    // 配置 GPTimer 基本属性
    gptimer_config_t timer_cfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // APB 时钟 (80 MHz)
        .direction = Config.count_up ? GPTIMER_COUNT_UP : GPTIMER_COUNT_DOWN,
        .resolution_hz = GPTIMER_RESOLUTION_HZ, // 1 MHz 分辨率，分频系数 = 80
        .intr_priority = 1,                     // 显式设置优先级
    };

    gptimer_handle_t handle = NULL;
    esp_err_t ret = gptimer_new_timer(&timer_cfg, &handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create timer: %s", esp_err_to_name(ret));
        return -1;
    }

    // 注册事件回调（必须在 gptimer_enable 之前）
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_isr_callback,
    };
    ret = gptimer_register_event_callbacks(handle, &cbs, pTimer);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register callbacks: %s", esp_err_to_name(ret));
        gptimer_del_timer(handle);
        return -1;
    }

    // 设置报警动作（计数到 alarm_count 触发回调）
    gptimer_alarm_config_t alarm_cfg = {
        .reload_count = 0, // 报警后重载的起始值
        .alarm_count = alarm_count,
        .flags.auto_reload_on_alarm = Config.auto_reload,
    };
    ret = gptimer_set_alarm_action(handle, &alarm_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set alarm: %s", esp_err_to_name(ret));
        gptimer_del_timer(handle);
        return -1;
    }

    // 保存实例状态（定时器未使能，需外部调用 timer_start）
    pTimer->handle = handle;
    pTimer->callback = Callback;
    pTimer->arg = CallbackArg;
    pTimer->config = Config;
    pTimer->is_running = false;

    ESP_LOGI(TAG, "Timer init OK, freq=%luHz, alarm_count=%llu",
             (unsigned long)Config.freq_hz, (unsigned long long)alarm_count);
    return 0;
}

/*********************************************************************************
 * @name          timer_start
 * @brief         使能并启动定时器计数
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：已初始化但未启动的实例指针
 * @return        返回值：代表含义
 *********************************************************************************/
void timer_start(TIMER_S *pTimer)
{
    if (!pTimer || !pTimer->is_used || !pTimer->handle)
    {
        ESP_LOGE(TAG, "Cannot start: invalid timer");
        return;
    }
    if (pTimer->is_running)
    {
        ESP_LOGW(TAG, "Timer already running");
        return;
    }

    esp_err_t ret = gptimer_enable(pTimer->handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable: %s", esp_err_to_name(ret));
        return;
    }

    ret = gptimer_start(pTimer->handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start: %s", esp_err_to_name(ret));
        gptimer_disable(pTimer->handle); // 回滚使能状态，保持一致性
        return;
    }
    pTimer->is_running = true;
    ESP_LOGI(TAG, "Timer started");
}

/*********************************************************************************
 * @name          timer_stop
 * @brief         停止定时器并禁用硬件
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：正在运行中的实例指针
 * @return        返回值：代表含义
 *********************************************************************************/

void timer_stop(TIMER_S *pTimer)
{
    if (!pTimer || !pTimer->is_used || !pTimer->handle)
    {
        ESP_LOGE(TAG, "Cannot stop: invalid timer");
        return;
    }
    if (!pTimer->is_running)
    {
        return;
    }

    gptimer_stop(pTimer->handle);
    gptimer_disable(pTimer->handle);
    pTimer->is_running = false;
    ESP_LOGI(TAG, "Timer stopped");
}

/*********************************************************************************
 * @name          timer_get_counter
 * @brief         获取定时器当前原始计数值（单位：tick）
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：已初始化的实例指针
 * @return        返回值：当前计数值，失败返回 0
 *********************************************************************************/

uint64_t timer_get_counter(TIMER_S *pTimer)
{
    if (!pTimer || !pTimer->is_used || !pTimer->handle)
    {
        return 0;
    }
    uint64_t count = 0;
    gptimer_get_raw_count(pTimer->handle, &count);
    return count;
}

/*********************************************************************************
 * @name          timer_deinit
 * @brief         反初始化定时器：停止、删除硬件资源，并标记槽位为空闲
 * @note          内部调用 timer_stop 确保安全关闭，随后释放 GPTimer 句柄。
 *                完成后 pTimer 可视为未分配状态，可直接丢弃或重新 alloc 使用。
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：已初始化的实例指针
 * @return        返回值：代表含义
 *********************************************************************************/
void timer_deinit(TIMER_S *pTimer)
{
    if (!pTimer || !pTimer->is_used || !pTimer->handle)
    {
        return;
    }

    timer_stop(pTimer);                // 停止并禁用（若未运行也安全）
    gptimer_del_timer(pTimer->handle); // 删除硬件定时器
    pTimer->handle = NULL;
    pTimer->is_used = false; // 标记槽位空闲
    pTimer->callback = NULL;
    ESP_LOGI(TAG, "Timer deinitialized");
}

/*********************************************************************************
 * @name          timer_is_running
 * @brief         查询定时器是否正在运行
 * @author        ouqingfeng
 * @date          2026/06/25
 * @version       v0.1
 * @param         pTimer：定时器实例指针
 * @return        返回值：true 运行中，false 未运行或指针无效
 *********************************************************************************/
bool timer_is_running(TIMER_S *pTimer)
{
    return (pTimer && pTimer->is_used && pTimer->is_running);
}
