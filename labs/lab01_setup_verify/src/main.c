/*
 * Lab 01 — Toolchain & Environment Verification
 * KMUTNB · M.Eng. Real-Time Operating Systems
 *
 * Two FreeRTOS tasks confirm the toolchain, RTOS, and UART are all working.
 * Optionally enables SEGGER SystemView tracing (build with: make SYSVIEW=1).
 *
 *   vBlinkTask     (HIGH prio, 500 ms)  — simulates LED blink over UART
 *   vHeartbeatTask (LOW  prio, 1000 ms) — prints uptime once per second
 *
 * Build:           make
 * Build+SysView:   make SYSVIEW=1   (copy SystemView src/ files first)
 * Flash:           make flash
 */

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "uart.h"

#ifdef USE_SYSVIEW
#include "SEGGER_SYSVIEW.h"
#endif

/* ── Clock setup ─────────────────────────────────────────────────────────────
 * MCXN236 boots from FRO 12 MHz.  Switch core to FRO 96 MHz (FIRC) so that
 * configCPU_CLOCK_HZ matches the actual oscillator before the scheduler runs.
 */
#define SCG0_BASE        0x40044000u
#define SYSCON0_BASE     0x40000000u
#define SCG_FIRCCSR      (*(volatile uint32_t *)(SCG0_BASE   + 0x300u))
#define SCG_RCCR         (*(volatile uint32_t *)(SCG0_BASE   + 0x010u))
#define SYSCON_AHBCLKDIV (*(volatile uint32_t *)(SYSCON0_BASE + 0x380u))

static void hw_init(void)
{
    SCG_FIRCCSR |= 1u;                              /* enable FRO 96 MHz */
    while (!(SCG_FIRCCSR & (1u << 24))) ;           /* wait until valid  */
    SYSCON_AHBCLKDIV = 0u;                          /* AHB divider = /1  */
    SCG_RCCR = (SCG_RCCR & ~(0xFu << 24)) | (3u << 24); /* SCS = FIRC   */
    uart_init();
}

/* ── Shared UART mutex ───────────────────────────────────────────────────── */
static SemaphoreHandle_t xUartMutex;

/* ── Tasks ───────────────────────────────────────────────────────────────── */

/*
 * vBlinkTask — HIGH priority, 500 ms period.
 * Simulates toggling an LED and reports the state over UART.
 * Uses vTaskDelay (simple delay, not drift-compensated — lab01 keeps it basic).
 */
static void vBlinkTask(void *pvParams)
{
    (void)pvParams;
    uint32_t count = 0;

    for (;;) {
        count++;
        const char *state = (count & 1u) ? "ON " : "OFF";

        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        uart_printf("[BLINK] LED %s  (toggle #%u)\n", state, count);
        xSemaphoreGive(xUartMutex);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/*
 * vHeartbeatTask — LOW priority, 1000 ms period.
 * Prints a live uptime counter — confirms the scheduler is running and that
 * lower-priority tasks still get CPU time.
 */
static void vHeartbeatTask(void *pvParams)
{
    (void)pvParams;

    for (;;) {
        uint32_t uptime_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        uart_printf("[HB]    uptime = %u ms\n", uptime_ms);
        xSemaphoreGive(xUartMutex);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ── Main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    hw_init();

    uart_puts("\n=== RTOS Lab 01: Toolchain & Environment Verify ===\n");
    uart_puts("Tasks: Blink (500 ms) | Heartbeat (1000 ms)\n");

#ifdef USE_SYSVIEW
    SEGGER_SYSVIEW_Conf();
    uart_puts("SystemView: enabled — open SEGGER SystemView and start recording\n");
#else
    uart_puts("SystemView: disabled (rebuild with: make SYSVIEW=1)\n");
#endif

    uart_puts("\n");

    xUartMutex = xSemaphoreCreateMutex();
    if (!xUartMutex) {
        uart_puts("FATAL: could not create mutex\n");
        for (;;) ;
    }

    xTaskCreate(vBlinkTask,     "Blink", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(vHeartbeatTask, "HB",    configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    uart_puts("[MAIN] Starting scheduler...\n\n");
    vTaskStartScheduler();

    for (;;) ;
}

/* ── FreeRTOS application hooks ─────────────────────────────────────────── */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    uart_printf("[FATAL] Stack overflow: %s\n", pcTaskName);
    for (;;) ;
}

void vApplicationMallocFailedHook(void)
{
    uart_puts("[FATAL] FreeRTOS heap exhausted\n");
    for (;;) ;
}
