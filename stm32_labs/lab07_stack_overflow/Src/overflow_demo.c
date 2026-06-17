#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "overflow_demo.h"

extern void uart_lock(void);
extern void uart_unlock(void);

/* ── Safe task — adequate stack size, periodic watermark report ──────────── */
void vSafeTask(void *pvParameters)
{
    (void)pvParameters;
    char buf[64]; /* a modest local allocation */

    for (;;) {
        snprintf(buf, sizeof(buf), "[Safe] watermark=%u words",
                 (unsigned)uxTaskGetStackHighWaterMark(NULL));
        uart_lock();
        printf("%s\r\n", buf);
        uart_unlock();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ── Recursive function that will overflow the stack ────────────────────── */
static void recurse(uint32_t depth)
{
    volatile uint8_t frame_data[64]; /* consume 64 bytes per stack frame */
    (void)frame_data;
    if (depth == 0) return;
    recurse(depth - 1);
}

/* ── Danger task — triggers a stack overflow intentionally ───────────────── */
void vDangerTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t depth = 0;

    for (;;) {
        depth++;
        uart_lock();
        printf("[Danger] recursion depth=%lu  watermark=%u words\r\n",
               (unsigned long)depth, (unsigned)uxTaskGetStackHighWaterMark(NULL));
        uart_unlock();

        recurse(depth);            /* deeper each cycle — will eventually overflow */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ── Monitor task — polls watermarks of all tasks ────────────────────────── */
void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(3000));

        /* Print task list with stack usage */
        static char task_list[512];
        vTaskList(task_list);       /* requires configUSE_TRACE_FACILITY 1 */

        uart_lock();
        printf("\r\n── Task Stack Watermarks ──\r\n%s\r\n", task_list);
        uart_unlock();
    }
}
