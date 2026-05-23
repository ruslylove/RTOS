/*
 * main.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 07 — Stack Overflow Detection & Memory Analysis
 *
 * Part A — Induce and detect a stack overflow with vOverflowTask.
 *   Step 1: configCHECK_FOR_STACK_OVERFLOW = 0  → observe crash without detection.
 *   Step 3: configCHECK_FOR_STACK_OVERFLOW = 2  → overflow hook fires.
 *   Stack: 64 words (256 bytes) for Step 1-2, 512 words for Part B onwards.
 *
 * Part B — Right-size all stacks with vStackReportTask.
 *
 * Part C — Heap and linker map analysis (no code change needed).
 *
 * Part D — Convert vLoggerTask to static allocation; measure heap change.
 *
 * Enable the Lab 03 tasks via ENABLE_PIPELINE below to get realistic HWM data
 * for the pipeline tasks (Sensor1, Sensor2, Logger, Monitor).
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

#include "FreeRTOSConfig.h"
#include "uart.h"
#include "overflow_demo.h"
#include "stack_sizing.h"

/* ── Part selection ──────────────────────────────────────────────────────── */
/* 1 = 64-word stack  (Part A crash demo / overflow hook)
 * 0 = 512-word stack (Part B high-water-mark measurement)           */
#define OVERFLOW_SMALL_STACK  1

/* Set to 1 to include the Lab 03 pipeline tasks for Part B HWM analysis */
#define ENABLE_PIPELINE  0

#if ENABLE_PIPELINE
#include "producer_consumer.h"
QueueHandle_t      xPipelineQueue  = NULL;
EventGroupHandle_t xPipelineEvents = NULL;
#endif

/* ── Task handles (needed by vStackReportTask) ───────────────────────────── */
TaskHandle_t xOverflowHandle = NULL;

#if ENABLE_PIPELINE
TaskHandle_t xSensor1Handle  = NULL;
TaskHandle_t xSensor2Handle  = NULL;
TaskHandle_t xLoggerHandle   = NULL;
TaskHandle_t xMonitorHandle  = NULL;
#endif

/* ── FreeRTOS hooks ─────────────────────────────────────────────────────── */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    uart_puts("[FATAL] heap allocation failed\r\n");
    for (;;) {}
}

/* vApplicationStackOverflowHook is defined in overflow_demo.c */

/* ════════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    /* TODO: hw_init() — configure PLL to 150 MHz, then call uart_init() */
    uart_init();

    uart_puts("\r\n=== RTOS Lab 07: Stack Overflow Detection & Memory Analysis ===\r\n");
    uart_printf("[MAIN] heap free at startup: %u bytes\r\n",
                (unsigned)xPortGetFreeHeapSize());

    /* ── Part A / B — overflow demonstration task ────────────────────────── */
#if OVERFLOW_SMALL_STACK
    /* 64 words = 256 bytes — intentionally too small to trigger overflow */
    xTaskCreate(vOverflowTask, "Overflow", 64, NULL, 2, &xOverflowHandle);
#else
    /* 512 words — large enough to run; used for Part B HWM measurement */
    xTaskCreate(vOverflowTask, "Overflow", 512, NULL, 2, &xOverflowHandle);
#endif

    /* ── Part B — stack high-water-mark reporter ──────────────────────────── */
    /* This task waits 30 s then prints HWM for all registered handles.      */
    xTaskCreate(vStackReportTask, "StackRpt", 256, NULL, 1, NULL);

#if ENABLE_PIPELINE
    /* ── Lab 03 pipeline tasks (for Part B HWM analysis) ─────────────────── */
    xPipelineQueue  = xQueueCreate(10, sizeof(SensorReading_t));
    xPipelineEvents = xEventGroupCreate();
    configASSERT(xPipelineQueue  != NULL);
    configASSERT(xPipelineEvents != NULL);

    xTaskCreate(vSensor1Task, "Sensor1",  256, NULL, 3, &xSensor1Handle);
    xTaskCreate(vSensor2Task, "Sensor2",  256, NULL, 2, &xSensor2Handle);
    xTaskCreate(vLoggerTask,  "Logger",   256, NULL, 1, &xLoggerHandle);
    xTaskCreate(vMonitorTask, "Monitor",  256, NULL, 2, &xMonitorHandle);
#endif

    uart_printf("[MAIN] starting scheduler — free heap: %u bytes\r\n",
                (unsigned)xPortGetFreeHeapSize());

    vTaskStartScheduler();
    for (;;) {}
}
