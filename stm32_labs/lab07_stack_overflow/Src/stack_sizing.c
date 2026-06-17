#include "FreeRTOS.h"
#include "task.h"
#include "stack_sizing.h"

/* Static stack allocation example */
#define STATIC_STACK_DEPTH  256

static StackType_t  xStaticStack[STATIC_STACK_DEPTH];
static StaticTask_t xStaticTCB;

/* Re-enable static allocation for this file only */
#undef configSUPPORT_STATIC_ALLOCATION
#define configSUPPORT_STATIC_ALLOCATION 1

extern void uart_lock(void);
extern void uart_unlock(void);

#include <stdio.h>

void vStaticAllocTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        UBaseType_t wm = uxTaskGetStackHighWaterMark(NULL);
        uart_lock();
        printf("[Static] stack watermark=%u / %u words remaining\r\n",
               (unsigned)wm, STATIC_STACK_DEPTH);
        uart_unlock();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* Helper: create vStaticAllocTask with a static stack buffer */
TaskHandle_t create_static_task(void)
{
    return xTaskCreateStatic(vStaticAllocTask, "StaticT",
                             STATIC_STACK_DEPTH, NULL, 1,
                             xStaticStack, &xStaticTCB);
}

UBaseType_t get_high_watermark(TaskHandle_t h)
{
    return uxTaskGetStackHighWaterMark(h);
}
