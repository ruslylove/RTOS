#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "watchdog.h"
#include "deadlock_tasks.h"

extern void uart_lock(void);
extern void uart_unlock(void);

/* ── Healthy task — always kicks both hw and sw watchdog ─────────────────── */
void vHealthyTask(void *pvParameters)
{
    (void)pvParameters;
    sw_watchdog_register(0, "Healthy", 1500);

    for (;;) {
        watchdog_kick();          /* refresh IWDG */
        sw_watchdog_kick(0);      /* refresh sw watchdog */

        uart_lock();
        printf("[Healthy] alive at t=%lu ms\r\n",
               (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
        uart_unlock();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ── Simulated deadlocked task — stops kicking after 10 s ───────────────── */
void vDeadlockTask(void *pvParameters)
{
    (void)pvParameters;
    sw_watchdog_register(1, "Deadlock", 3000);
    TickType_t xStart = xTaskGetTickCount();

    for (;;) {
        uint32_t elapsed = (xTaskGetTickCount() - xStart) * portTICK_PERIOD_MS;

        if (elapsed < 10000) {
            sw_watchdog_kick(1);
            uart_lock();
            printf("[Deadlock] kicking at t=%lu ms\r\n", (unsigned long)elapsed);
            uart_unlock();
        } else {
            /* Simulate hang — task is stuck, stops kicking */
            uart_lock();
            printf("[Deadlock] HUNG at t=%lu ms — waiting for hw reset\r\n",
                   (unsigned long)elapsed);
            uart_unlock();
            for (;;) { /* hung */ }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ── Monitor task — checks sw watchdogs + kicks IWDG ────────────────────── */
void vWatchdogMonitor(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        sw_watchdog_check();
        /* Do NOT kick IWDG here — vHealthyTask owns the IWDG kick.
           If vHealthyTask hangs, IWDG will expire and reset the MCU. */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
