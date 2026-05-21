/*
 * Lab 02 — FreeRTOS Basics on FRDM-MCXN236
 * KMUTNB · M.Eng. Real-Time Operating Systems
 *
 * Demonstrates: tasks, queue, binary semaphore, mutex, software timer.
 */

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "uart.h"
#include "lab_tasks.h"

/* ── Clock setup ────────────────────────────────────────────────────────────
 * MCXN236 boots from FRO 12 MHz.  We switch the core to FRO 96 MHz (FRO_HF)
 * before starting the scheduler so that configCPU_CLOCK_HZ matches reality.
 *
 * Registers used:
 *   SCG0     base 0x40044000
 *   SYSCON0  base 0x40000000
 */
#define SCG0_BASE   0x40044000u
#define SYSCON0_BASE 0x40000000u

/* SCG FIRCCSR: bit 0 = FIRC enable, bit 24 = FIRC valid (read-only) */
#define SCG_FIRCCSR     (*(volatile uint32_t *)(SCG0_BASE + 0x300u))
/* SCG RCCR: system clock source for RUN mode */
#define SCG_RCCR        (*(volatile uint32_t *)(SCG0_BASE + 0x10u))
/* SYSCON AHBCLKDIV: system clock divider */
#define SYSCON_AHBCLKDIV (*(volatile uint32_t *)(SYSCON0_BASE + 0x380u))

static void hw_init(void)
{
    /* Enable FRO 96 MHz (FIRC) — SCG_FIRCCSR bit 0 */
    SCG_FIRCCSR |= 1u;
    /* Wait until FIRC is valid (bit 24) */
    while (!(SCG_FIRCCSR & (1u << 24)))
        ;

    /* Set AHB clock divider to 1 (no divide) */
    SYSCON_AHBCLKDIV = 0u;

    /* Switch system clock to FIRC (FRO 96 MHz): RCCR SCS field = 3 */
    SCG_RCCR = (SCG_RCCR & ~(0xFu << 24)) | (3u << 24);

    /* Now initialise UART (uses FRO 96 MHz via FCCLKSEL[4]=3) */
    uart_init();
}

/* ── FreeRTOS shared objects ─────────────────────────────────────────────── */
/* Defined here; declared extern in lab_tasks.h */
QueueHandle_t    xSensorQueue;
SemaphoreHandle_t xUartMutex;
SemaphoreHandle_t xStatusSem;

/* ── Task priorities ─────────────────────────────────────────────────────── */
#define PRIO_SENSOR    3   /* highest application priority */
#define PRIO_DISPLAY   2
#define PRIO_STATUS    1   /* lowest application priority */

int main(void)
{
    hw_init();

    uart_puts("\n=== RTOS Lab 02: FreeRTOS Basics ===\n");
    uart_puts("Tasks: Sensor | Display | Status\n");
    uart_puts("Primitives: Queue | Mutex | Semaphore | Timer\n\n");

    /* Create RTOS objects */
    xSensorQueue = xQueueCreate(8, sizeof(SensorReading_t));
    xUartMutex   = xSemaphoreCreateMutex();
    xStatusSem   = xSemaphoreCreateBinary();

    if (!xSensorQueue || !xUartMutex || !xStatusSem) {
        uart_puts("FATAL: failed to create RTOS objects\n");
        for (;;) ;
    }

    /* Create tasks */
    xTaskCreate(vSensorTask,  "Sensor",  256, NULL, PRIO_SENSOR,  NULL);
    xTaskCreate(vDisplayTask, "Display", 256, NULL, PRIO_DISPLAY, NULL);
    xTaskCreate(vStatusTask,  "Status",  256, NULL, PRIO_STATUS,  NULL);

    /* Create software timer — 1000 ms, auto-reload */
    TimerHandle_t xStatusTimer = xTimerCreate(
        "StatusTimer",
        pdMS_TO_TICKS(1000),
        pdTRUE,             /* auto-reload */
        NULL,
        vStatusTimerCallback
    );
    xTimerStart(xStatusTimer, 0);

    uart_puts("[MAIN] Starting scheduler...\n\n");
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;) ;
}

/* ── FreeRTOS application hooks ─────────────────────────────────────────── */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    uart_printf("[FATAL] Stack overflow in task: %s\n", pcTaskName);
    for (;;) ;
}

void vApplicationMallocFailedHook(void)
{
    uart_puts("[FATAL] FreeRTOS heap exhausted\n");
    for (;;) ;
}
