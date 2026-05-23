/*
 * inversion_tasks.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 04 — Priority Inversion: See It, Fix It
 *
 * PART A — Binary Semaphore (no PIP): inversion occurs.
 * In main.c create xSharedResource with:
 *     xSharedResource = xSemaphoreCreateBinary();
 *     xSemaphoreGive(xSharedResource);   // mark as available
 *
 * PART C — Mutex (PIP enabled): replace the create lines with:
 *     xSharedResource = xSemaphoreCreateMutex();
 * No other code changes required — the xSemaphoreTake/Give API is identical.
 *
 * Timing offsets ensure tL grabs the resource first, then tH arrives, and
 * finally tM preempts tL — creating the classic three-task inversion window.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "inversion_tasks.h"
#include "uart.h"

/* ── Task parameters ─────────────────────────────────────────────────────── */
#define TASK_H_PERIOD_MS      200
#define TASK_M_PERIOD_MS      150
#define TASK_L_PERIOD_MS      300
#define CS_H_MS               5     /* tH critical section length */
#define CS_L_MS               40    /* tL critical section length */
#define COMPUTE_M_MS          30    /* tM computation (no resource) */

/*
 * Startup delay offsets — chosen so that on the first period:
 *   tL acquires the resource at t ~= 0 ms
 *   tH arrives at t ~= 100 ms (while tL holds the resource)
 *   tM runs at t ~= 50 ms  (between tL acquire and tH arrival)
 */
#define OFFSET_H_MS           100
#define OFFSET_M_MS           50
#define OFFSET_L_MS           0

/* ══════════════════════════════════════════════════════════════════════════ */
/* tH — High priority task (priority 3)                                      */
/* ══════════════════════════════════════════════════════════════════════════ */

void vTaskH(void *pvParameters)
{
    (void)pvParameters;

    /* Initial offset so tL grabs the resource before tH arrives */
    vTaskDelay(pdMS_TO_TICKS(OFFSET_H_MS));

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_H_PERIOD_MS));

        TickType_t t_arrive = xTaskGetTickCount();
        uart_printf("[H] arrived at %lu ms\r\n", t_arrive);

        /* TODO: Take the shared resource (may block if tL holds it) */
        xSemaphoreTake(xSharedResource, portMAX_DELAY);

        TickType_t t_acquire = xTaskGetTickCount();
        uart_printf("[H] acquired resource at %lu ms  (waited %lu ms)\r\n",
                    t_acquire, t_acquire - t_arrive);

        /* Critical section */
        vTaskDelay(pdMS_TO_TICKS(CS_H_MS));

        /* TODO: Release the shared resource */
        xSemaphoreGive(xSharedResource);

        uart_printf("[H] released resource\r\n");
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* tM — Medium priority task (priority 2) — no shared resource               */
/* ══════════════════════════════════════════════════════════════════════════ */

void vTaskM(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(OFFSET_M_MS));

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_M_PERIOD_MS));

        uart_printf("[M] running (no resource held)  t=%lu ms\r\n",
                    (uint32_t)xTaskGetTickCount());

        /*
         * Simulate 30 ms of computation using a busy-wait.
         * Using vTaskDelay would relinquish the CPU — we need tM to stay
         * runnable so it blocks tL via preemption (the inversion scenario).
         */
        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(COMPUTE_M_MS))
        {
            /* busy wait */
        }

        uart_printf("[M] done  t=%lu ms\r\n", (uint32_t)xTaskGetTickCount());
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* tL — Low priority task (priority 1)                                        */
/* ══════════════════════════════════════════════════════════════════════════ */

void vTaskL(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(OFFSET_L_MS));

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_L_PERIOD_MS));

        /* TODO: Acquire the shared resource */
        xSemaphoreTake(xSharedResource, portMAX_DELAY);

        uart_printf("[L] acquired resource -- holding for %d ms  t=%lu ms\r\n",
                    CS_L_MS, (uint32_t)xTaskGetTickCount());

        /*
         * Simulate 40 ms critical section with a busy-wait.
         * tL must remain runnable (not sleeping) so that when tH arrives
         * and blocks on xSharedResource, the inversion window is created
         * when tM then preempts tL.
         */
        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(CS_L_MS))
        {
            /* busy wait */
        }

        /* TODO: Release the shared resource */
        xSemaphoreGive(xSharedResource);

        uart_printf("[L] released resource  t=%lu ms\r\n",
                    (uint32_t)xTaskGetTickCount());
    }
}
