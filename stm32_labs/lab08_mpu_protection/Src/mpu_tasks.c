#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "mpu_config.h"
#include "mpu_tasks.h"

extern void uart_lock(void);
extern void uart_unlock(void);

/* ── Normal task: reads (not writes) the protected buffer — allowed ─────── */
void vNormalTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t count = 0;

    for (;;) {
        /* Reading a RO region is permitted — only writes fault */
        uart_lock();
        printf("[Normal #%lu] secure_buffer = \"%.*s\"\r\n",
               (unsigned long)++count, (int)sizeof(secure_buffer), (char *)secure_buffer);
        uart_unlock();

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ── Monitor task: prints task-list table every 4 s ─────────────────────── */
void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(4000));

        static char task_list[512];
        vTaskList(task_list);

        uart_lock();
        printf("\r\n── Task States ──\r\n%s\r\n", task_list);
        uart_unlock();
    }
}

/* ── Rogue task: intentionally writes to the protected buffer ────────────── */
#if USE_ROGUE_TASK
void vRogueTask(void *pvParameters)
{
    (void)pvParameters;

    /* Let the system print a few normal cycles first */
    vTaskDelay(pdMS_TO_TICKS(3000));

    uart_lock();
    printf("\r\n[Rogue] Attempting WRITE to protected region 0x%08lX ...\r\n",
           (unsigned long)secure_buffer);
    uart_unlock();

    /* This write violates the MPU Read-Only region → MemManage fault fires */
    secure_buffer[0] = 0xAA;

    /* Never reached */
    uart_lock();
    printf("[Rogue] Write succeeded (MPU not working?)\r\n");
    uart_unlock();
    vTaskDelete(NULL);
}
#endif /* USE_ROGUE_TASK */
