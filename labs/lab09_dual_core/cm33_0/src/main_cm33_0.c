/*
 * main_cm33_0.c — Primary core (CM33_0) entry point
 * Lab 09 — Dual-Core AMP: Messaging Unit & Shared SRAM
 *
 * CM33_0 responsibilities:
 *   - Boot CM33_1 by writing its firmware start address into SYSCON
 *   - Create ADC producer task
 *   - Send data to CM33_1 via Messaging Unit (MU)
 */
#include "FreeRTOS.h"
#include "task.h"
#include "fsl_debug_console.h"
#include "fsl_mu.h"
#include "board.h"
#include "adc_producer.h"
#include "shared_mem.h"

/* Shared ring buffer (placed in SRAM4 via linker) */
RingBuf_t    g_ring    __attribute__((section(".sram4")));
LatencyTest_t g_latency __attribute__((section(".sram4")));

static TaskHandle_t xADCTaskHandle = NULL;

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    PRINTF("[CM33_0] FATAL: malloc failed\r\n");
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    taskDISABLE_INTERRUPTS();
    PRINTF("[CM33_0] FATAL: stack overflow: %s\r\n", pcTaskName);
    for (;;) {}
}

int main(void)
{
    BOARD_InitHardware();

    PRINTF("\r\n=== RTOS Lab 09: Dual-Core AMP (CM33_0) ===\r\n");

    /* ── Enable MU clock and initialise peripheral ──────────────────────── */
    CLOCK_EnableClock(kCLOCK_Mu0A);
    MU_Init(MUA);

    /* ── Part A: release CM33_1 from reset ──────────────────────────────── */
    /*
     * TODO (Part A): write CM33_1 boot address to SYSCON and release reset.
     *
     * On MCXN236 the second core is held in reset by default.  To release it:
     *   1. Write the CM33_1 vector table address (firmware start) to the
     *      appropriate SYSCON register (check the RM for CPU1_VTOR or CPUCTRL).
     *   2. Clear the CM33_1 reset bit in SYSCON->CPUCTRL.
     *
     * Example (verify register names against the MCXN236 RM):
     *   SYSCON->CPUCTRL = ((CM33_1_BOOT_ADDR >> 1) | 0x01U);
     *
     * The CM33_1 firmware must be flashed separately (see cm33_1/Makefile).
     */
    PRINTF("[CM33_0] NOTE: CM33_1 release not yet implemented (see TODO in main)\r\n");

    /* ── Initialise shared memory ────────────────────────────────────────── */
    g_ring.write_idx = 0U;
    g_ring.read_idx  = 0U;
    g_ring.missed    = 0U;

    g_latency.send_cycles = 0U;
    g_latency.recv_cycles = 0U;

    /* DMB: shared memory init must be visible before CM33_1 starts */
    __DMB();

    /* ── Create tasks ────────────────────────────────────────────────────── */
    xTaskCreate(vADCProducerTask, "ADCProd", 512, NULL, 3, &xADCTaskHandle);

    PRINTF("[CM33_0] starting scheduler — free heap: %u bytes\r\n",
           (unsigned)xPortGetFreeHeapSize());

    vTaskStartScheduler();
    for (;;) {}
}
