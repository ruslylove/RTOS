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
#include "fsl_debug_console.h"
#include "fsl_mu.h"
#include "board.h"
#include "fir_consumer.h"
#include "shared_mem.h"

/*
 * g_ring and g_latency are defined (allocated) in cm33_0/src/main_cm33_0.c
 * and placed in SRAM4 (.sram4 section).  CM33_1 accesses them as externs —
 * no separate storage is allocated here.
 */

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

    /* ── Enable MU clock and initialise slave side ──────────────────────── */
    CLOCK_EnableClock(kCLOCK_Mu0B);
    MU_Init(MUB);

    /* ── Create consumer task ────────────────────────────────────────────── */
    xTaskCreate(vFIRConsumerTask, "FIRCons", 512, NULL, 3, NULL);

    PRINTF("[CM33_1] starting scheduler — free heap: %u bytes\r\n",
           (unsigned)xPortGetFreeHeapSize());

    vTaskStartScheduler();
    for (;;) {}
}
