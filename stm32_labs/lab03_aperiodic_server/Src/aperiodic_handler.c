#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "aperiodic_handler.h"

SemaphoreHandle_t xAperiodicSem = NULL;
QueueHandle_t     xRequestQueue = NULL;

extern void uart_lock(void);
extern void uart_unlock(void);

static uint32_t s_event_count = 0;

/* ── Part A — Deferred Interrupt Handler pattern ────────────────────────── */
void vAperiodicHandlerTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        /* Block indefinitely until an "interrupt" signals the semaphore */
        xSemaphoreTake(xAperiodicSem, portMAX_DELAY);

        uart_lock();
        printf("[APERIODIC] event #%lu handled at t=%lu ms\r\n",
               (unsigned long)++s_event_count,
               (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
        uart_unlock();

        /* Simulate ~2 ms of work */
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

/* ── Part B — Polling Server ─────────────────────────────────────────────── */
void vPollingServerTask(void *pvParameters)
{
    (void)pvParameters;
    AperiodicRequest_t req;
    TickType_t xLastWake = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(100); /* server period Ts */

    for (;;) {
        /* Serve all pending requests within this server period */
        while (xQueueReceive(xRequestQueue, &req, 0) == pdTRUE) {
            uart_lock();
            printf("[PS] serving event_id=%lu (queued at t=%lu ms)\r\n",
                   (unsigned long)req.event_id,
                   (unsigned long)req.timestamp_ms);
            uart_unlock();
            vTaskDelay(pdMS_TO_TICKS(1)); /* simulate service */
        }
        vTaskDelayUntil(&xLastWake, xPeriod);
    }
}

/* ── Simulated event generator (call from a low-priority task or timer) ── */
void vSimulateAperiodicEvent(void)
{
    static uint32_t id = 0;

    if (xAperiodicSem) {
        xSemaphoreGive(xAperiodicSem);
    }

    if (xRequestQueue) {
        AperiodicRequest_t req = {
            .event_id    = ++id,
            .timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS
        };
        xQueueSend(xRequestQueue, &req, 0);
    }
}
