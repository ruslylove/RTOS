/*
 * Lab 02 — FreeRTOS Basics: queue, semaphore, software timer
 * vSensorTask  — produces simulated ADC readings onto xSensorQueue every 500 ms
 * vDisplayTask — consumes readings, prints via UART (protected by mutex)
 * vStatusTask  — waits for timer semaphore, prints system status every 1 s
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "lab_tasks.h"

QueueHandle_t     xSensorQueue;
SemaphoreHandle_t xUartMutex;
SemaphoreHandle_t xStatusSem;

/* ── Sensor task — simulates ADC, sends to queue ─────────────────────────── */
void vSensorTask(void *pvParameters)
{
    (void)pvParameters;
    static uint32_t ch = 0;
    static uint32_t val = 1024;

    for (;;) {
        SensorReading_t r;
        r.channel      = ch++ % 4;
        r.raw_value    = val;
        r.timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

        /* Simulate sawtooth on each channel */
        val = (val + 137) & 0xFFF;

        xQueueSend(xSensorQueue, &r, pdMS_TO_TICKS(10));

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ── Display task — dequeues and prints readings ────────────────────────── */
void vDisplayTask(void *pvParameters)
{
    (void)pvParameters;
    SensorReading_t r;

    for (;;) {
        if (xQueueReceive(xSensorQueue, &r, portMAX_DELAY) == pdTRUE) {
            xSemaphoreTake(xUartMutex, portMAX_DELAY);
            printf("[SENSOR] ch=%lu  raw=%4lu  t=%lu ms\r\n",
                   (unsigned long)r.channel,
                   (unsigned long)r.raw_value,
                   (unsigned long)r.timestamp_ms);
            xSemaphoreGive(xUartMutex);
        }
    }
}

/* ── Status task — blocks on binary semaphore given by software timer ──── */
void vStatusTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t count = 0;

    for (;;) {
        xSemaphoreTake(xStatusSem, portMAX_DELAY);
        count++;

        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        printf("[STATUS] tick #%lu  free_heap=%lu B  uptime=%lu ms\r\n",
               (unsigned long)count,
               (unsigned long)xPortGetFreeHeapSize(),
               (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
        xSemaphoreGive(xUartMutex);
    }
}

/* ── Timer callback — gives semaphore to wake status task ───────────────── */
void vStatusTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xStatusSem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
