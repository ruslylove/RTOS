#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "periodic_tasks.h"

extern void uart_lock(void);
extern void uart_unlock(void);

void vPeriodic1Task(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;) {
        uart_lock();
        printf("[P1] tick=%lu\r\n", (unsigned long)xTaskGetTickCount());
        uart_unlock();
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(500));
    }
}

void vPeriodic2Task(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;) {
        uart_lock();
        printf("[P2] tick=%lu\r\n", (unsigned long)xTaskGetTickCount());
        uart_unlock();
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(1000));
    }
}
