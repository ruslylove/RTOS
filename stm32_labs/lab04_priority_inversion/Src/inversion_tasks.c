/*
 * Lab 04 — Priority Inversion
 *
 * Classic three-task scenario:
 *   LOW  (prio 1) acquires xSharedResource, does CPU work
 *   MED  (prio 3) runs CPU-bound work — preempts LOW, blocks HIGH
 *   HIGH (prio 5) waits for xSharedResource held by LOW
 *
 * With binary semaphore: HIGH can be blocked for the duration of MED's work.
 * With mutex: FreeRTOS priority inheritance raises LOW's priority to 5
 *             so it can complete quickly and release to HIGH.
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "inversion_tasks.h"

SemaphoreHandle_t xSharedResource = NULL;

static void busy_work(uint32_t ms)
{
    /* Spin-loop to consume CPU without blocking — simulates non-blocking work */
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(ms)) {
        __asm volatile("nop");
    }
}

extern void uart_lock(void);
extern void uart_unlock(void);

static void print_event(const char *task, const char *event)
{
    uart_lock();
    printf("[%s] %s at t=%lu ms\r\n", task, event,
           (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
    uart_unlock();
}

/* ── LOW priority task ───────────────────────────────────────────────────── */
void vLowPrioTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000)); /* wait before next cycle */

        print_event("LOW", "acquiring resource");
        xSemaphoreTake(xSharedResource, portMAX_DELAY);
        print_event("LOW", "resource ACQUIRED, doing work (200 ms)");

        busy_work(200); /* holds resource while doing CPU work */

        print_event("LOW", "releasing resource");
        xSemaphoreGive(xSharedResource);
    }
}

/* ── MEDIUM priority task ────────────────────────────────────────────────── */
void vMedPrioTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5100)); /* start slightly after LOW */
        print_event("MED", "running CPU work (150 ms) — preempts LOW");
        busy_work(150);
        print_event("MED", "CPU work done");
    }
}

/* ── HIGH priority task ──────────────────────────────────────────────────── */
void vHighPrioTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5050)); /* starts between LOW and MED */

        print_event("HIGH", "requesting resource");
        TickType_t t0 = xTaskGetTickCount();
        xSemaphoreTake(xSharedResource, portMAX_DELAY);
        TickType_t blocked_ms = (xTaskGetTickCount() - t0) * portTICK_PERIOD_MS;

        uart_lock();
        printf("[HIGH] got resource after %lu ms blocking\r\n",
               (unsigned long)blocked_ms);
        uart_unlock();

        /* Use resource briefly */
        vTaskDelay(pdMS_TO_TICKS(10));
        xSemaphoreGive(xSharedResource);
        print_event("HIGH", "resource released");
    }
}
