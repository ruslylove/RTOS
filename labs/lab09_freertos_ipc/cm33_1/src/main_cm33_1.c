/*
 * main_cm33_1.c — Secondary core (CM33_1) entry point
 * Lab 09 — Dual-Core AMP: Messaging Unit & Shared SRAM
 *
 * CM33_1 responsibilities:
 *   - Initialise MU peripheral (slave side)
 *   - Create FIR consumer task
 *   - Wait on MU notifications from CM33_0, drain ring buffer, filter samples
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "app.h"
#include "fir_consumer.h"
#include "shared_mem.h"

/*
 * On a true dual-core device these live in shared SRAM and are defined on CM33_0.
 * On single-core MCXN236, cm33_0 already contains the complete functional demo;
 * cm33_1 exists as a reference build. We allocate locals here so it links cleanly.
 */
RingBuf_t     g_ring;
LatencyTest_t g_latency;
QueueHandle_t g_notify_queue;

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    PRINTF("[CM33_1] FATAL: malloc failed\r\n");
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    taskDISABLE_INTERRUPTS();
    PRINTF("[CM33_1] FATAL: stack overflow: %s\r\n", pcTaskName);
    for (;;) {}
}

int main(void)
{
    BOARD_InitHardware();

    PRINTF("\r\n=== RTOS Lab 09: Dual-Core AMP (CM33_1) ===\r\n");

    /* ── Create consumer task ────────────────────────────────────────────── */
    xTaskCreate(vFIRConsumerTask, "FIRCons", 512, NULL, 3, NULL);

    PRINTF("[CM33_1] starting scheduler — free heap: %u bytes\r\n",
           (unsigned)xPortGetFreeHeapSize());

    vTaskStartScheduler();
    for (;;) {}
}
