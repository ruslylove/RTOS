#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32h5xx_hal.h"
#include "power_tasks.h"

extern void uart_lock(void);
extern void uart_unlock(void);

volatile uint32_t ulIdleCount = 0U;
volatile uint32_t ulTickCount = 0U;

/* ── LED blink task ──────────────────────────────────────────────────────── */
void vLedTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t blinks = 0;

    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {
        .Pin = GPIO_PIN_13, .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW
    };
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED off at start */

    for (;;) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); /* LED on  */
        vTaskDelay(pdMS_TO_TICKS(50));
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   /* LED off */
        vTaskDelay(pdMS_TO_TICKS(LED_PERIOD_MS - 50U));

        uart_lock();
        printf("[LED  ] blink #%lu  tick=%lu\r\n",
               (unsigned long)++blinks,
               (unsigned long)xTaskGetTickCount());
        uart_unlock();
    }
}

/* ── Simulated sensor task ───────────────────────────────────────────────── */
void vSensorTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t reads = 0;

    for (;;) {
        /* Block for 2 s — during this time the system can sleep */
        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));

        /* Simulate a brief burst of active work (~1 ms) */
        volatile uint32_t i;
        for (i = 0; i < 25000U; i++) { __NOP(); }

        uart_lock();
        printf("[Sensr] read #%lu (2 s period)\r\n", (unsigned long)++reads);
        uart_unlock();
    }
}

/* ── Statistics task ─────────────────────────────────────────────────────── */
void vStatsTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(STATS_PERIOD_MS));

        /* Snapshot and reset counters atomically */
        taskENTER_CRITICAL();
        uint32_t idle_cnt = ulIdleCount; ulIdleCount = 0U;
        uint32_t tick_cnt = ulTickCount; ulTickCount = 0U;
        taskEXIT_CRITICAL();

        /*
         * With tickless idle, SysTick is suppressed during sleep; fewer
         * tick interrupts fire.  tick_cnt reflects actual tick ISR executions
         * per STATS_PERIOD_MS window.  Maximum would be STATS_PERIOD_MS ticks
         * (one per ms) if the CPU never slept.
         *
         * CPU utilisation estimate: ticks fired / expected ticks * 100 %
         * (rough proxy — each tick that fires means the CPU was running that ms)
         */
        uint32_t expected_ticks = STATS_PERIOD_MS; /* 1 ms tick rate */
        uint32_t active_pct = (tick_cnt * 100U) / expected_ticks;
        if (active_pct > 100U) active_pct = 100U;  /* clamp (int overshoot) */

        uart_lock();
        printf("\r\n── Power Stats (last %u s) ──\r\n"
               "  Idle entries   : %lu\r\n"
               "  Tick ISR fires : %lu / %lu expected\r\n"
               "  Est. CPU active: ~%lu%%\r\n\r\n",
               (unsigned)(STATS_PERIOD_MS / 1000U),
               (unsigned long)idle_cnt,
               (unsigned long)tick_cnt,
               (unsigned long)expected_ticks,
               (unsigned long)active_pct);
        uart_unlock();
    }
}
