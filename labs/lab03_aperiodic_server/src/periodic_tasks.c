/*
 * periodic_tasks.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 03 — Aperiodic Servers & Producer-Consumer Pipeline
 *
 * Background periodic tasks that simulate continuous workload on the CPU.
 * They print a short marker each period so you can observe interleaving with
 * aperiodic event handling in the terminal output.
 *
 * tau1 — vPeriodic1Task : T = 100 ms, priority 3
 * tau2 — vPeriodic2Task : T = 200 ms, priority 2
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "periodic_tasks.h"
#include "uart.h"

/* ── tau1: T = 100 ms, priority 3 ─────────────────────────────────────────── */
void vPeriodic1Task(void *pvParameters)
{
    (void)pvParameters;

    static uint32_t count = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        /* Wait until the next 100 ms period boundary */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));

        count++;
        uart_printf("[P1] period #%lu  t=%lu ms\r\n",
                    count, (uint32_t)xTaskGetTickCount());

        /* TODO (optional): add a short busy-wait here to simulate
         * computation load and measure its effect on aperiodic response time.
         * Example:
         *   volatile uint32_t i;
         *   for (i = 0; i < 15000; i++) {}   // ~100 us at 150 MHz
         */
    }
}

/* ── tau2: T = 200 ms, priority 2 ─────────────────────────────────────────── */
void vPeriodic2Task(void *pvParameters)
{
    (void)pvParameters;

    static uint32_t count = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));

        count++;
        uart_printf("[P2] period #%lu  t=%lu ms\r\n",
                    count, (uint32_t)xTaskGetTickCount());
    }
}
