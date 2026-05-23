/*
 * main.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 05 — Deadlock Detection & Watchdog Timers
 *
 * Enable one section at a time by setting LAB_PART:
 *   1 -> Part A (deadlock)
 *   2 -> Part B (resource ordering fix)
 *   3 -> Part C (hardware watchdog)
 *   4 -> Part D (software watchdog monitor)
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "uart.h"
#include "deadlock_tasks.h"
#include "watchdog.h"

/* NXP SDK for reset-cause detection */
#include "fsl_wdog32.h"

/* Select the lab part to run (1–4) */
#ifndef LAB_PART
#define LAB_PART  1
#endif

/* ── Kernel object handles ───────────────────────────────────────────────── */
SemaphoreHandle_t xMutexA = NULL;
SemaphoreHandle_t xMutexB = NULL;

/* ── FreeRTOS hooks ─────────────────────────────────────────────────────── */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    uart_puts("[FATAL] malloc failed\r\n");
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    taskDISABLE_INTERRUPTS();
    uart_printf("[FATAL] stack overflow: %s\r\n", pcTaskName);
    for (;;) {}
}

/* ════════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    /* TODO: hw_init() — configure PLL to 150 MHz */
    uart_init();

    uart_puts("\r\n=== RTOS Lab 05: Deadlock & Watchdog ===\r\n");

    /* ── Check watchdog reset cause at boot (Part C) ────────────────────── */
#if LAB_PART >= 3
    /* After a watchdog reset this flag is set — print a diagnostic message */
    if (WDOG32_GetStatusFlags(WDOG0) & kWDOG32_RunningFlag)
    {
        /* TODO: check the correct flag for "reset was caused by watchdog"
         * Refer to the MCXN236 RM: WDOG32_CS[DONE] or use
         * RCM_GetPreviousResetSources() from the SDK.                     */
    }
    uart_puts("[BOOT] checking reset cause...\r\n");
    /* Placeholder — replace with actual reset cause detection */
    uart_puts("[BOOT] (no watchdog reset detected on this boot)\r\n");
#endif

    /* ── Parts A & B — Deadlock tasks ───────────────────────────────────── */
#if LAB_PART == 1 || LAB_PART == 2
    uart_printf("[MAIN] Part %d — creating mutex pair and deadlock tasks\r\n",
                LAB_PART);

    xMutexA = xSemaphoreCreateMutex();
    xMutexB = xSemaphoreCreateMutex();
    configASSERT(xMutexA != NULL && xMutexB != NULL);

    xTaskCreate(vTaskAlpha, "Alpha", 256, NULL, 2, NULL);
    xTaskCreate(vTaskBeta,  "Beta",  256, NULL, 2, NULL);
#endif

    /* ── Part C — Hardware watchdog ─────────────────────────────────────── */
#if LAB_PART == 3
    uart_puts("[MAIN] Part C — hardware watchdog task\r\n");
    /* Priority 4 — highest, so it always runs before the deadline */
    xTaskCreate(vWatchdogTask, "Watchdog", 256, NULL, 4, NULL);
#endif

    /* ── Part D — Software watchdog monitor ─────────────────────────────── */
#if LAB_PART == 4
    uart_puts("[MAIN] Part D — software watchdog monitor\r\n");
    xTaskCreate(vWatchdogMonitor, "WdgMon", 256, NULL, 4, NULL);

    /* Create NUM_WORKERS worker tasks, passing the ID as the parameter */
    for (uintptr_t i = 0; i < NUM_WORKERS; i++)
    {
        xTaskCreate(vWorkerTask, "Worker", 256, (void *)i, 2, NULL);
    }
#endif

    uart_printf("[MAIN] starting scheduler — free heap: %u bytes\r\n",
                (unsigned)xPortGetFreeHeapSize());

    vTaskStartScheduler();
    for (;;) {}
}
