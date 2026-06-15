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
#include "board.h"
#include "app.h"
#include "fsl_debug_console.h"
#include "deadlock_tasks.h"
#include "watchdog.h"

#include "fsl_wwdt.h"

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
    PRINTF("[FATAL] malloc failed\r\n");
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    taskDISABLE_INTERRUPTS();
    PRINTF("[FATAL] stack overflow: %s\r\n", pcTaskName);
    for (;;) {}
}

/* ════════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    BOARD_InitHardware();

    PRINTF("\r\n=== RTOS Lab 05: Deadlock & Watchdog ===\r\n");

    /* ── Check watchdog reset cause at boot (Part C) ────────────────────── */
#if LAB_PART >= 3
    PRINTF("[BOOT] checking reset cause...\r\n");
    if (WWDT_GetStatusFlags(WWDT0) & kWWDT_TimeoutFlag)
    {
        PRINTF("[BOOT] *** last reset was caused by WWDT timeout ***\r\n");
        WWDT_ClearStatusFlags(WWDT0, kWWDT_TimeoutFlag);
    }
    else
    {
        PRINTF("[BOOT] (no watchdog reset on this boot)\r\n");
    }
#endif

    /* ── Parts A & B — Deadlock tasks ───────────────────────────────────── */
#if LAB_PART == 1 || LAB_PART == 2
    PRINTF("[MAIN] Part %d — creating mutex pair and deadlock tasks\r\n",
                LAB_PART);

    xMutexA = xSemaphoreCreateMutex();
    xMutexB = xSemaphoreCreateMutex();
    configASSERT(xMutexA != NULL && xMutexB != NULL);

    xTaskCreate(vTaskAlpha, "Alpha", 256, NULL, 2, NULL);
    xTaskCreate(vTaskBeta,  "Beta",  256, NULL, 2, NULL);
#endif

    /* ── Part C — Hardware watchdog ─────────────────────────────────────── */
#if LAB_PART == 3
    PRINTF("[MAIN] Part C — hardware watchdog task\r\n");
    /* Priority 4 — highest, so it always runs before the deadline */
    xTaskCreate(vWatchdogTask, "Watchdog", 256, NULL, 4, NULL);
#endif

    /* ── Part D — Software watchdog monitor ─────────────────────────────── */
#if LAB_PART == 4
    PRINTF("[MAIN] Part D — software watchdog monitor\r\n");
    xTaskCreate(vWatchdogMonitor, "WdgMon", 256, NULL, 4, NULL);

    /* Create NUM_WORKERS worker tasks, passing the ID as the parameter */
    for (uintptr_t i = 0; i < NUM_WORKERS; i++)
    {
        xTaskCreate(vWorkerTask, "Worker", 256, (void *)i, 2, NULL);
    }
#endif

    PRINTF("[MAIN] starting scheduler — free heap: %u bytes\r\n",
                (unsigned)xPortGetFreeHeapSize());

    vTaskStartScheduler();
    for (;;) {}
}
