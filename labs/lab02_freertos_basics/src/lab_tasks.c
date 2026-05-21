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
 *     Prints a system status report whenever the software timer
 *     fires and gives xStatusSem.
 *
 *   Software timer (1000 ms, auto-reload)
 *     Gives xStatusSem from its callback, waking vStatusTask.
 *
 * Concepts demonstrated
 *   Tasks & scheduling  — three tasks with different priorities
 *   Queue               — xSensorQueue (Sensor → Display)
 *   Semaphore           — xStatusSem (timer ISR-context → task)
 *   Mutex               — xUartMutex (UART critical section)
 */

#include "lab_tasks.h"
#include "uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* ──────────────────────────────────────────────────────────────────────────
 * vSensorTask — HIGH priority
 * Period: 500 ms, uses vTaskDelayUntil for drift-free timing.
 * Sends one SensorReading_t per period to xSensorQueue.
 * ────────────────────────────────────────────────────────────────────────── */
void vSensorTask(void *pvParams)
{
    (void)pvParams;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(500);
    int32_t raw = 0;

    for (;;) {
        /* Simulate an ADC reading that oscillates between 0 and 100 */
        raw = (raw + 7) % 101;

        SensorReading_t reading = {
            .timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS,
            .value        = raw,
        };

        /* Non-blocking send: if queue is full, the reading is dropped */
        if (xQueueSend(xSensorQueue, &reading, 0) != pdTRUE) {
            /* Queue full — take mutex and warn once */
            if (xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                uart_puts("[SENSOR] Queue full — reading dropped!\n");
                xSemaphoreGive(xUartMutex);
            }
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* ──────────────────────────────────────────────────────────────────────────
 * vDisplayTask — MEDIUM priority
 * Blocks on xSensorQueue indefinitely.  When a reading arrives it locks
 * xUartMutex and prints, then releases the mutex.
 * ────────────────────────────────────────────────────────────────────────── */
void vDisplayTask(void *pvParams)
{
    (void)pvParams;

    SensorReading_t reading;

    for (;;) {
        /* Block until a reading arrives */
        if (xQueueReceive(xSensorQueue, &reading, portMAX_DELAY) == pdTRUE) {

            /* Mutex: protect UART from concurrent access by vStatusTask */
            xSemaphoreTake(xUartMutex, portMAX_DELAY);
            uart_printf("[DISPLAY] t=%u ms  value=%d\n",
                        reading.timestamp_ms, reading.value);
            xSemaphoreGive(xUartMutex);
        }
    }
}

/* ──────────────────────────────────────────────────────────────────────────
 * vStatusTask — LOW priority
 * Blocks on xStatusSem.  The software timer gives the semaphore every 1 s,
 * waking this task to print a status snapshot.
 * ────────────────────────────────────────────────────────────────────────── */
void vStatusTask(void *pvParams)
{
    (void)pvParams;

    for (;;) {
        /* Block until the timer fires */
        xSemaphoreTake(xStatusSem, portMAX_DELAY);

        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        uart_printf("\n[STATUS] uptime=%u ms  queue_waiting=%u\n",
                    xTaskGetTickCount() * portTICK_PERIOD_MS,
                    uxQueueMessagesWaiting(xSensorQueue));
        xSemaphoreGive(xUartMutex);
    }
}

/* ──────────────────────────────────────────────────────────────────────────
 * vStatusTimerCallback — runs in timer-task context (not an ISR).
 * Gives xStatusSem to wake vStatusTask.
 * ────────────────────────────────────────────────────────────────────────── */
void vStatusTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    xSemaphoreGive(xStatusSem);
}
