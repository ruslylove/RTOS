/*
 * main.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 06 — WCET Measurement with DWT & Interrupt Latency
 *
 * Part A: vWcetMeasureTask — isolates and measures three workload functions.
 * Part B: vGpioToggleTask + GPIO1_IRQHandler — measures ISR latency.
 * Part C: vSortTask — measures bubble_sort in a multi-task context.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "dwt.h"
#include "workload.h"
#include "latency_test.h"

/* Select which parts to run */
#define ENABLE_PART_A   1   /* WCET measurement task */
/* #define ENABLE_PART_B   1 */   /* ISR latency test */
/* #define ENABLE_PART_C   1 */   /* periodic sort in context */

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

    /* Enable DWT cycle counter before any task runs */
    DWT_Enable();

    PRINTF("\r\n=== RTOS Lab 06: WCET Measurement with DWT ===\r\n\r\n");

#ifdef ENABLE_PART_A
    /* Part A: one-shot measurement task */
    xTaskCreate(vWcetMeasureTask, "WcetMeas", 512, NULL, 3, NULL);
#endif

#ifdef ENABLE_PART_B
    /* Part B: ISR latency measurement */
    latency_test_init();
    xTaskCreate(vGpioToggleTask,     "GpioTog",   256, NULL, 3, NULL);
    xTaskCreate(vLatencyReportTask,  "LatReport",  256, NULL, 1, NULL);
    /* Uncomment to add a critical section that increases worst-case latency */
    /* xTaskCreate(vCriticalSectionTask, "CritSec", 256, NULL, 2, NULL); */
#endif

#ifdef ENABLE_PART_C
    /* Part C: periodic bubble_sort alongside background tasks */
    xTaskCreate(vSortTask, "Sort",     512, NULL, 3, NULL);
    /* Two background tasks at lower priorities */
    /* xTaskCreate(vBackgroundTask1, "BG1", 256, NULL, 1, NULL); */
    /* xTaskCreate(vBackgroundTask2, "BG2", 256, NULL, 2, NULL); */
#endif

    PRINTF("[MAIN] starting scheduler — free heap: %u bytes\r\n",
                (unsigned)xPortGetFreeHeapSize());

    vTaskStartScheduler();
    for (;;) {}
}
