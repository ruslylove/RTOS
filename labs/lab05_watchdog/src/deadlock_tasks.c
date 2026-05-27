/*
 * deadlock_tasks.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 05, Parts A & B
 *
 * Part A — Deadlock (opposite lock order):
 *   Alpha: MutexA -> MutexB
 *   Beta:  MutexB -> MutexA   <- circular wait -> deadlock
 *
 * Part B — Prevention (resource ordering):
 *   Set RESOURCE_ORDER_FIX to 1. Both tasks acquire MutexA before MutexB.
 *   The circular-wait Coffman condition is broken.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "deadlock_tasks.h"
#include "fsl_debug_console.h"

/* Set to 0 for Part A (deadlock), 1 for Part B (resource ordering fix) */
#define RESOURCE_ORDER_FIX  0

/* ══════════════════════════════════════════════════════════════════════════ */
/* vTaskAlpha — priority 2                                                    */
/* Acquire order: MutexA (lock order 1) then MutexB (lock order 2) — correct */
/* ══════════════════════════════════════════════════════════════════════════ */

void vTaskAlpha(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(10));

        PRINTF("[A] taking MutexA...\r\n");
        xSemaphoreTake(xMutexA, portMAX_DELAY);
        PRINTF("[A] took MutexA. Taking MutexB...\r\n");

        /* Brief delay — creates a window for Beta to acquire MutexB */
        vTaskDelay(pdMS_TO_TICKS(5));

        xSemaphoreTake(xMutexB, portMAX_DELAY);
        PRINTF("[A] took both -- doing work  t=%lu ms\r\n",
                    (uint32_t)xTaskGetTickCount());

        /* Release in reverse acquisition order */
        xSemaphoreGive(xMutexB);
        xSemaphoreGive(xMutexA);
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* vTaskBeta — priority 2                                                     */
/* Part A: MutexB then MutexA  (wrong order -> deadlock with Alpha)          */
/* Part B: MutexA then MutexB  (correct order -> no deadlock)                */
/* ══════════════════════════════════════════════════════════════════════════ */

void vTaskBeta(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(10));

#if RESOURCE_ORDER_FIX
        /* Part B — FIXED: acquire in ascending lock order (A before B) */
        PRINTF("[B] taking MutexA (order fix)...\r\n");
        xSemaphoreTake(xMutexA, portMAX_DELAY);
        PRINTF("[B] taking MutexB...\r\n");
        vTaskDelay(pdMS_TO_TICKS(5));
        xSemaphoreTake(xMutexB, portMAX_DELAY);
        PRINTF("[B] took both -- doing work  t=%lu ms\r\n",
                    (uint32_t)xTaskGetTickCount());
        xSemaphoreGive(xMutexB);
        xSemaphoreGive(xMutexA);
#else
        /* Part A — BROKEN: opposite order creates circular wait */
        PRINTF("[B] taking MutexB...\r\n");
        xSemaphoreTake(xMutexB, portMAX_DELAY);
        PRINTF("[B] took MutexB. Taking MutexA...\r\n");
        vTaskDelay(pdMS_TO_TICKS(5));
        /* TODO: observe that this line stalls permanently after deadlock */
        xSemaphoreTake(xMutexA, portMAX_DELAY);   /* <- opposite order to Alpha */
        PRINTF("[B] took both -- doing work\r\n");
        xSemaphoreGive(xMutexA);
        xSemaphoreGive(xMutexB);
#endif
    }
}
