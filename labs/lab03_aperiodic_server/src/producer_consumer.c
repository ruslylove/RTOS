/*
 * producer_consumer.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 03, Part C — Two-Producer, One-Consumer Pipeline
 *
 * Data flow:
 *   vSensor1Task (100 ms)  ─┐
 *                           ├── xPipelineQueue ──> vLoggerTask ──> xPipelineEvents
 *   vSensor2Task (200 ms)  ─┘                                          │
 *                                                               vMonitorTask
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "producer_consumer.h"
#include "fsl_debug_console.h"

/* ══════════════════════════════════════════════════════════════════════════ */
/* Sensor 1 — Producer, T = 100 ms, priority 3                               */
/* ══════════════════════════════════════════════════════════════════════════ */

void vSensor1Task(void *pvParameters)
{
    (void)pvParameters;

    static int16_t raw = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));

        /* Generate a synthetic reading (wraps at 1000) */
        raw = (int16_t)((raw + 7) % 1000);

        SensorReading_t r = {
            .sensor_id = 1,
            .value     = raw,
            .timestamp = xTaskGetTickCount()
        };

        /* TODO: Send to pipeline queue (non-blocking — drop if full) */
        if (xQueueSend(xPipelineQueue, &r, 0) != pdTRUE)
        {
            PRINTF("[S1] queue full -- dropped reading val=%d\r\n", raw);
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Sensor 2 — Producer, T = 200 ms, priority 2                               */
/* ══════════════════════════════════════════════════════════════════════════ */

void vSensor2Task(void *pvParameters)
{
    (void)pvParameters;

    static int16_t raw = 500;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));

        raw = (int16_t)((raw + 13) % 1000);

        SensorReading_t r = {
            .sensor_id = 2,
            .value     = raw,
            .timestamp = xTaskGetTickCount()
        };

        /* TODO: Send to pipeline queue */
        xQueueSend(xPipelineQueue, &r, 0);
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Logger — Consumer, priority 1                                              */
/* ══════════════════════════════════════════════════════════════════════════ */

void vLoggerTask(void *pvParameters)
{
    (void)pvParameters;

    static uint32_t count = 0;
    SensorReading_t r;

    for (;;)
    {
        /* TODO: Block indefinitely until a reading arrives */
        xQueueReceive(xPipelineQueue, &r, portMAX_DELAY);

        count++;
        PRINTF("[LOG] #%u  sensor=%d  val=%d  t=%u ms\r\n",
                    count, r.sensor_id, r.value, r.timestamp);

        /* TODO: Set BIT_ALARM if value exceeds threshold */
        if (r.value > 800)
        {
            xEventGroupSetBits(xPipelineEvents, BIT_ALARM);
        }

        /* TODO: Set BIT_DATA_READY every 10 items */
        if ((count % 10) == 0)
        {
            xEventGroupSetBits(xPipelineEvents, BIT_DATA_READY);
        }

        /*
         * Question C3 experiment: uncomment to slow down the logger and
         * observe queue saturation and dropped readings from Sensor 1.
         *   vTaskDelay(pdMS_TO_TICKS(150));
         */
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Monitor — Event group waiter, priority 2                                   */
/* ══════════════════════════════════════════════════════════════════════════ */

void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /*
         * TODO: Wait for BIT_DATA_READY OR BIT_ALARM (OR = pdFALSE).
         * Clear the bits on exit (pdTRUE).
         */
        EventBits_t bits = xEventGroupWaitBits(
            xPipelineEvents,
            BIT_DATA_READY | BIT_ALARM,
            pdTRUE,    /* clear bits on exit */
            pdFALSE,   /* OR: either bit wakes us */
            portMAX_DELAY);

        if (bits & BIT_DATA_READY)
        {
            PRINTF("[MON] 10-item batch complete  t=%u ms\r\n",
                        (uint32_t)xTaskGetTickCount());
        }

        if (bits & BIT_ALARM)
        {
            PRINTF("[MON] ALARM: value exceeded threshold  t=%u ms\r\n",
                        (uint32_t)xTaskGetTickCount());
        }
    }
}
