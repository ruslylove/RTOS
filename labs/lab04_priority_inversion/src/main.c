/*
 * main.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 04 — Priority Inversion: See It, Fix It
 *
 * Part A (binary semaphore — inversion occurs):
 *   xSharedResource = xSemaphoreCreateBinary();
 *   xSemaphoreGive(xSharedResource);
 *
 * Part C (mutex — PIP prevents inversion):
 *   xSharedResource = xSemaphoreCreateMutex();
 *
 * Switch between the two by changing the USE_MUTEX define below.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "uart.h"
#include "inversion_tasks.h"

/* ── Part selection ──────────────────────────────────────────────────────── */
/* 0 = binary semaphore (Part A — demonstrates inversion)
 * 1 = mutex            (Part C — PIP prevents inversion)  */
#define USE_MUTEX  0

/* ── Kernel object handles ───────────────────────────────────────────────── */
SemaphoreHandle_t xSharedResource = NULL;
SemaphoreHandle_t xResource2      = NULL;   /* Experiment D1 only */

/* ── FreeRTOS hook implementations ──────────────────────────────────────── */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    uart_puts("[FATAL] malloc failed\r\n");
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    taskDISABLE_INTERRUPTS();
    uart_printf("[FATAL] stack overflow: %s\r\n", pcTaskName);
    for (;;) {}
}

/* ════════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    /* TODO: hw_init() — clock to 150 MHz, uart_init() */
    uart_init();

    uart_puts("\r\n=== RTOS Lab 04: Priority Inversion ===\r\n");
#if USE_MUTEX
    uart_puts("[MAIN] mode: MUTEX (PIP enabled)\r\n\r\n");
#else
    uart_puts("[MAIN] mode: BINARY SEMAPHORE (inversion expected)\r\n\r\n");
#endif

    /* ── Create the shared resource ─────────────────────────────────────── */
#if USE_MUTEX
    /* Part C: mutex enables Priority Inheritance Protocol */
    xSharedResource = xSemaphoreCreateMutex();
#else
    /* Part A: binary semaphore — no PIP */
    xSharedResource = xSemaphoreCreateBinary();
    xSemaphoreGive(xSharedResource);   /* initialise as available */
#endif
    configASSERT(xSharedResource != NULL);

    /* ── Create tasks ───────────────────────────────────────────────────── */
    xTaskCreate(vTaskH, "TaskH", 256, NULL, 3, NULL);
    xTaskCreate(vTaskM, "TaskM", 256, NULL, 2, NULL);
    xTaskCreate(vTaskL, "TaskL", 256, NULL, 1, NULL);

    uart_printf("[MAIN] starting scheduler — free heap: %u bytes\r\n",
                (unsigned)xPortGetFreeHeapSize());

    vTaskStartScheduler();
    for (;;) {}
}
