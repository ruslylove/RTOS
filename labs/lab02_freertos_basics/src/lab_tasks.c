/*
 * Lab 02 — FreeRTOS task demonstrations.
 *
 * Scenario: "Sensor Data Pipeline"
 *
 *   vSensorTask  (HIGH prio, 500 ms period)
 *     Simulates a sensor reading and sends it to xSensorQueue.
 *     Uses vTaskDelayUntil() to achieve a precise period.
 *
 *   vDisplayTask  (MED prio, blocks on queue)
 *     Receives readings from xSensorQueue and prints them.
 *     Acquires xUartMutex before printing to prevent interleaving.
 *
 *   vStatusTask  (LOW prio, blocks on semaphore)
 *     Prints a system status report whenever the software timer fires.
 *
 *   Software timer (1000 ms, auto-reload)
 *     Gives xStatusSem from its callback, waking vStatusTask.
 *
 * Concepts demonstrated
 *   Tasks & scheduling  — three tasks with different priorities
 *   Queue               — xSensorQueue (Sensor → Display)
 *   Semaphore           — xStatusSem (timer callback → task)
 *   Mutex               — xUartMutex (PRINTF critical section)
 */

#include "lab_tasks.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "fsl_debug_console.h"

/* ── vSensorTask ─────────────────────────────────────────────────────────────
 * HIGH priority, 500 ms period via vTaskDelayUntil.
 * Sends one SensorReading_t per period to xSensorQueue.
 * ──────────────────────────────────────────────────────────────────────────── */
void vSensorTask(void *pvParams)
{
    (void)pvParams;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(500);
    int32_t raw = 0;

    for (;;) {
        raw = (raw + 7) % 101;   /* oscillates 0..100 */

        SensorReading_t reading = {
            .timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS,
            .value        = raw,
        };

        if (xQueueSend(xSensorQueue, &reading, 0) != pdTRUE) {
            if (xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                PRINTF("[SENSOR] Queue full — reading dropped!\r\n");
                xSemaphoreGive(xUartMutex);
            }
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* ── vDisplayTask ────────────────────────────────────────────────────────────
 * MEDIUM priority. Blocks on xSensorQueue indefinitely.
 * Locks xUartMutex before printing to avoid interleaving with vStatusTask.
 * ──────────────────────────────────────────────────────────────────────────── */
void vDisplayTask(void *pvParams)
{
    (void)pvParams;

    SensorReading_t reading;

    for (;;) {
        if (xQueueReceive(xSensorQueue, &reading, portMAX_DELAY) == pdTRUE) {
            xSemaphoreTake(xUartMutex, portMAX_DELAY);
            PRINTF("[DISPLAY] t=%u ms  value=%d\r\n",
                   (unsigned)reading.timestamp_ms, (int)reading.value);
            xSemaphoreGive(xUartMutex);
        }
    }
}

/* ── vStatusTask ─────────────────────────────────────────────────────────────
 * LOW priority. Blocks on xStatusSem.
 * The software timer gives the semaphore every 1 s, waking this task.
 * ──────────────────────────────────────────────────────────────────────────── */
void vStatusTask(void *pvParams)
{
    (void)pvParams;

    for (;;) {
        xSemaphoreTake(xStatusSem, portMAX_DELAY);

        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        PRINTF("\r\n[STATUS] uptime=%u ms  queue_waiting=%u\r\n",
               (unsigned)(xTaskGetTickCount() * portTICK_PERIOD_MS),
               (unsigned)uxQueueMessagesWaiting(xSensorQueue));
        xSemaphoreGive(xUartMutex);
    }
}

/* ── vStatusTimerCallback ────────────────────────────────────────────────────
 * Runs in timer-task context (not an ISR).
 * Gives xStatusSem to wake vStatusTask.
 * ──────────────────────────────────────────────────────────────────────────── */
void vStatusTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    xSemaphoreGive(xStatusSem);
}
