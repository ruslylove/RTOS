/*
 * static_alloc.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 07, Part D — Static Task Allocation
 *
 * Replaces the dynamic xTaskCreate() call for vLoggerTask with
 * xTaskCreateStatic(), placing the TCB and stack in .bss rather than the heap.
 *
 * Before enabling this:
 *   1. Set configSUPPORT_STATIC_ALLOCATION = 1 in FreeRTOSConfig.h.
 *   2. Measure the right-sized stack depth from Part B.
 *   3. Call Static_CreateLoggerTask() instead of the xTaskCreate() for Logger
 *      in main.c, and compare xPortGetFreeHeapSize() before and after.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "static_alloc.h"
#include "uart.h"

/*
 * Import vLoggerTask from producer_consumer.c.
 * If you are running Lab 07 standalone (without the Lab 03 pipeline), replace
 * this extern with a minimal logger stub.
 */
extern void vLoggerTask(void *pvParameters);

/* ── Static storage for the Logger task ─────────────────────────────────── */
static StaticTask_t xLoggerTaskBuffer;
static StackType_t  xLoggerStack[STATIC_LOGGER_STACK_WORDS];

/*
 * Static_CreateLoggerTask
 *
 * Creates vLoggerTask using static allocation. The TCB and stack come from
 * the static arrays above — no heap allocation is performed.
 *
 * Returns the task handle (or NULL if creation fails, which cannot happen with
 * xTaskCreateStatic as long as the buffers are valid and non-NULL).
 */
TaskHandle_t Static_CreateLoggerTask(void)
{
    TaskHandle_t handle;

    uart_printf("[STATIC] heap before static Logger: %u bytes free\r\n",
                (unsigned)xPortGetFreeHeapSize());

    handle = xTaskCreateStatic(
        vLoggerTask,           /* task function          */
        "Logger",              /* name                   */
        STATIC_LOGGER_STACK_WORDS, /* stack depth (words) */
        NULL,                  /* parameter              */
        1,                     /* priority               */
        xLoggerStack,          /* stack buffer           */
        &xLoggerTaskBuffer     /* TCB buffer             */
    );

    uart_printf("[STATIC] heap after  static Logger: %u bytes free\r\n",
                (unsigned)xPortGetFreeHeapSize());

    return handle;
}

/* ── Idle task static memory ────────────────────────────────────────────── */

/*
 * vApplicationGetIdleTaskMemory
 *
 * FreeRTOS calls this function to get the memory for the Idle task when
 * configSUPPORT_STATIC_ALLOCATION = 1. It must be defined exactly once.
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t  **ppxIdleTaskStackBuffer,
                                    uint32_t      *pulIdleTaskStackSize)
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t  xIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = xIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}
