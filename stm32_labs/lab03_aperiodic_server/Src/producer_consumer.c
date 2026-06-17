#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "producer_consumer.h"

QueueHandle_t      xPipelineQueue  = NULL;
EventGroupHandle_t xPipelineEvents = NULL;

extern void uart_lock(void);
extern void uart_unlock(void);

/* ── Sensor producers ─────────────────────────────────────────────────────── */
void vSensor1Task(void *pvParameters)
{
    (void)pvParameters;
    static uint32_t val = 512;
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;) {
        SensorReading_t r = { .sensor_id = 1, .raw = val,
                               .timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS };
        val = (val + 73) & 0xFFF;
        xQueueSend(xPipelineQueue, &r, pdMS_TO_TICKS(10));
        xEventGroupSetBits(xPipelineEvents, EV_SENSOR1_READY);
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(300));
    }
}

void vSensor2Task(void *pvParameters)
{
    (void)pvParameters;
    static uint32_t val = 2048;
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;) {
        SensorReading_t r = { .sensor_id = 2, .raw = val,
                               .timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS };
        val = (val + 211) & 0xFFF;
        xQueueSend(xPipelineQueue, &r, pdMS_TO_TICKS(10));
        xEventGroupSetBits(xPipelineEvents, EV_SENSOR2_READY);
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(700));
    }
}

/* ── Logger — drains the queue ──────────────────────────────────────────── */
void vLoggerTask(void *pvParameters)
{
    (void)pvParameters;
    SensorReading_t r;

    for (;;) {
        /* Wait for at least one sensor to post data (OR wait) */
        xEventGroupWaitBits(xPipelineEvents,
                            EV_SENSOR1_READY | EV_SENSOR2_READY,
                            pdTRUE,   /* clear bits on exit */
                            pdFALSE,  /* wait for ANY bit */
                            portMAX_DELAY);

        while (xQueueReceive(xPipelineQueue, &r, 0) == pdTRUE) {
            uart_lock();
            printf("[LOG] sensor=%lu  raw=%4lu  t=%lu ms\r\n",
                   (unsigned long)r.sensor_id,
                   (unsigned long)r.raw,
                   (unsigned long)r.timestamp_ms);
            uart_unlock();
        }
        xEventGroupSetBits(xPipelineEvents, EV_LOG_DONE);
    }
}

/* ── Monitor — waits for log completion, prints stats ──────────────────── */
void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t cycle = 0;

    for (;;) {
        xEventGroupWaitBits(xPipelineEvents, EV_LOG_DONE, pdTRUE, pdTRUE, portMAX_DELAY);
        cycle++;
        uart_lock();
        printf("[MON] cycle=%lu  free_heap=%lu B\r\n",
               (unsigned long)cycle, (unsigned long)xPortGetFreeHeapSize());
        uart_unlock();
    }
}
