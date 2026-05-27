/*
 * main_ns.c -- Non-Secure FreeRTOS application entry point
 *
 * Creates:
 *   vSensorTask    -- calls Secure_ADC_Read() via NSC veneer (Part B)
 *   vBenchmarkTask -- measures NSC vs direct call overhead (Part D)
 *
 * Also installs a SecureFault handler (Part C).
 *
 * Lab 08 -- TrustZone-M: Secure/Non-Secure Partition
 * Platform: NXP FRDM-MCXN236 (Cortex-M33 @ 150 MHz, Non-Secure)
 */

#include "FreeRTOS.h"
#include "task.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "MCXN236.h"
#include "ns_tasks.h"

/* uart.h shim is available for ns_tasks.c and other files that include it */

/* ── SecureFault handler (Part C) ────────────────────────────────────────── */
/*
 * The SecureFault exception fires when NS code attempts an illegal access into
 * the Secure address space (e.g., reading Secure SRAM directly).
 *
 * The handler runs in NS state.  It reads SFSR/SFAR from the SAU registers,
 * which are banked and accessible from NS for diagnostic purposes.
 *
 * TODO (Part C): Deliberately trigger this handler by reading from 0x20010000
 *   in vSensorTask, then observe the SFSR and SFAR values printed below.
 */
void SecureFault_Handler(void)
{
    PRINTF("[NS] !!! SecureFault !!!\r\n");

    /* TODO: print SFSR -- Secure Fault Status Register */
    PRINTF("[NS] SFSR = 0x%08lX\r\n", (unsigned long)SAU->SFSR);

    /* TODO: print SFAR -- Secure Fault Address Register (valid when SFARVALID=1) */
    PRINTF("[NS] SFAR = 0x%08lX\r\n", (unsigned long)SAU->SFAR);

    /* Spin -- do not attempt to return from a fault */
    for (;;) {}
}

/* ── vApplicationMallocFailedHook ────────────────────────────────────────── */
void vApplicationMallocFailedHook(void)
{
    PRINTF("[NS] FATAL: heap allocation failed\r\n");
    for (;;) {}
}

/* ── vApplicationStackOverflowHook ──────────────────────────────────────── */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    PRINTF("[NS] FATAL: stack overflow in task '%s'\r\n", pcTaskName);
    for (;;) {}
}

/* ── main ────────────────────────────────────────────────────────────────── */
int main(void)
{
    BOARD_InitHardware();

    PRINTF("[NS] starting FreeRTOS...\r\n");

    /* ── Task creation ──────────────────────────────────────────────────── */

    /* vSensorTask -- calls Secure_ADC_Read every 200 ms (Part B) */
    xTaskCreate(vSensorTask,
                "Sensor",
                256,            /* stack words */
                NULL,
                3,              /* priority */
                NULL);

    /* vBenchmarkTask -- runs once, measures NSC call overhead (Part D) */
    xTaskCreate(vBenchmarkTask,
                "Bench",
                512,
                NULL,
                2,
                NULL);

    /* ── Start scheduler ────────────────────────────────────────────────── */
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;) {}
}
