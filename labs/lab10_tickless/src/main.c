/*
 * main.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 10 — Tickless Idle Power Optimisation
 *
 * Part A: configUSE_TICKLESS_IDLE = 0 (standard SysTick ticking).
 *   Measure baseline current draw with a multimeter on J1 (IDD jumper).
 *
 * Part B: configUSE_TICKLESS_IDLE = 1 (LPTMR-backed tickless sleep).
 *   Implement vPortSuppressTicksAndSleep in tickless_port.c.
 *   Measure current draw again and compare timing accuracy via UART output.
 *
 * Part C: extend to VLPS stop mode — lower current at the cost of longer
 *   wakeup latency.  Compare LED toggle jitter between modes.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "adc_task.h"
#include "led_toggle_task.h"
#include "tickless_port.h"

/* ── FreeRTOS hooks ─────────────────────────────────────────────────────── */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    PRINTF("[FATAL] malloc failed\r\n");
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    taskDISABLE_INTERRUPTS();
    PRINTF("[FATAL] stack overflow: %s\r\n", pcTaskName);
    for (;;) {}
}

/* ════════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    BOARD_InitHardware();

    PRINTF("\r\n=== RTOS Lab 10: Tickless Idle Power Optimisation ===\r\n");

#if configUSE_TICKLESS_IDLE == 0
    PRINTF("[MAIN] Part A — standard SysTick ticking (baseline)\r\n");
#else
    PRINTF("[MAIN] Part B — tickless idle enabled (LPTMR wakeup)\r\n");
    /* Initialise LPTMR for tickless wakeup before the scheduler starts */
    LPTMR_Init_Tickless();
#endif

    PRINTF("[MAIN] free heap at startup: %u bytes\r\n",
           (unsigned)xPortGetFreeHeapSize());

    /* ── ADC sampling task — 500 ms period, priority 2 ──────────────────── */
    xTaskCreate(vADCTask,  "ADC",  256, NULL, 2, NULL);

    /* ── LED toggle task — 1 s period, priority 1 ───────────────────────── */
    xTaskCreate(vLEDTask,  "LED",  256, NULL, 1, NULL);

    PRINTF("[MAIN] starting scheduler — free heap: %u bytes\r\n",
           (unsigned)xPortGetFreeHeapSize());

    vTaskStartScheduler();
    for (;;) {}
}
