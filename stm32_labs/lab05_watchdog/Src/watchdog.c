#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32h5xx_hal.h"
#include "watchdog.h"

/* ── Hardware IWDG ───────────────────────────────────────────────────────── */
static IWDG_HandleTypeDef hiwdg;

void watchdog_init(void)
{
    /*
     * LSI ≈ 32000 Hz, prescaler 256 → counter freq ≈ 125 Hz
     * Reload = 249 → timeout = (249+1) / 125 ≈ 2.0 s
     */
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    hiwdg.Init.Reload    = 249;
    hiwdg.Init.Window    = IWDG_WINDOW_DISABLE;
    HAL_IWDG_Init(&hiwdg);
}

void watchdog_kick(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}

/* ── Software per-task watchdog ──────────────────────────────────────────── */
static TaskWatchdog_t s_watched[MAX_WATCHED_TASKS];

void sw_watchdog_register(uint8_t slot, const char *name, uint32_t deadline_ms)
{
    if (slot >= MAX_WATCHED_TASKS) return;
    s_watched[slot].name         = name;
    s_watched[slot].deadline_ms  = deadline_ms;
    s_watched[slot].last_kick_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    s_watched[slot].alive        = 1;
}

void sw_watchdog_kick(uint8_t slot)
{
    if (slot >= MAX_WATCHED_TASKS) return;
    taskENTER_CRITICAL();
    s_watched[slot].last_kick_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    s_watched[slot].alive        = 1;
    taskEXIT_CRITICAL();
}

void sw_watchdog_check(void)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    for (int i = 0; i < MAX_WATCHED_TASKS; i++) {
        if (!s_watched[i].name) continue;
        uint32_t elapsed = now - s_watched[i].last_kick_ms;
        if (elapsed > s_watched[i].deadline_ms && s_watched[i].alive) {
            s_watched[i].alive = 0;
            printf("[WDG] Task '%s' missed deadline! elapsed=%lu ms\r\n",
                   s_watched[i].name, (unsigned long)elapsed);
        }
    }
}
