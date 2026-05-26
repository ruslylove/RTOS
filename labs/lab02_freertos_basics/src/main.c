/*
 * Lab 02 — FreeRTOS Basics on FRDM-MCXN236
 * KMUTNB · M.Eng. Real-Time Operating Systems
 *
 * Demonstrates: tasks, queue, binary semaphore, mutex, software timer.
 * Uses NXP MCUXpresso SDK: BOARD_InitHardware() and PRINTF from fsl_debug_console.
 *
 *   vSensorTask  (HIGH prio, 500 ms) — simulates ADC readings → queue
 *   vDisplayTask (MED  prio, queue)  — receives readings, prints via UART
 *   vStatusTask  (LOW  prio, sem)    — prints system status every 1 s
 *   StatusTimer  (1000 ms auto-reload) — gives xStatusSem each second
 *
 * Build:  make
 * Flash:  make flash
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "app.h"

#include "lab_tasks.h"

/* ── Shared RTOS objects (declared extern in lab_tasks.h) ───────────────────── */
QueueHandle_t     xSensorQueue;
SemaphoreHandle_t xUartMutex;
SemaphoreHandle_t xStatusSem;

/* ── Task priorities ─────────────────────────────────────────────────────────── */
#define PRIO_SENSOR    3
#define PRIO_DISPLAY   2
#define PRIO_STATUS    1

int main(void)
{
    BOARD_InitHardware();

    PRINTF("\r\n=== RTOS Lab 02: FreeRTOS Basics ===\r\n");
    PRINTF("Tasks: Sensor | Display | Status\r\n");
    PRINTF("Primitives: Queue | Mutex | Semaphore | Timer\r\n\r\n");

    xSensorQueue = xQueueCreate(8, sizeof(SensorReading_t));
    xUartMutex   = xSemaphoreCreateMutex();
    xStatusSem   = xSemaphoreCreateBinary();

    if (!xSensorQueue || !xUartMutex || !xStatusSem) {
        PRINTF("FATAL: failed to create RTOS objects\r\n");
        for (;;) ;
    }

    xTaskCreate(vSensorTask,  "Sensor",  512, NULL, PRIO_SENSOR,  NULL);
    xTaskCreate(vDisplayTask, "Display", 512, NULL, PRIO_DISPLAY, NULL);
    xTaskCreate(vStatusTask,  "Status",  512, NULL, PRIO_STATUS,  NULL);

    TimerHandle_t xStatusTimer = xTimerCreate(
        "StatusTimer",
        pdMS_TO_TICKS(1000),
        pdTRUE,
        NULL,
        vStatusTimerCallback
    );
    xTimerStart(xStatusTimer, 0);

    PRINTF("[MAIN] Starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    for (;;) ;
}

/* ── FreeRTOS application hooks ─────────────────────────────────────────────── */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    PRINTF("[FATAL] Stack overflow: %s\r\n", pcTaskName);
    for (;;) ;
}

void vApplicationMallocFailedHook(void)
{
    PRINTF("[FATAL] FreeRTOS heap exhausted\r\n");
    for (;;) ;
}
