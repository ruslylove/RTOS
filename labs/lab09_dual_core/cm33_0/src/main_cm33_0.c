/*
 * main_cm33_0.c — Lab 09 main entry point
 * Lab 09 — Producer-Consumer IPC with FreeRTOS Queue
 *
 * MCXN236 is a single-core device; the dual-core concept is demonstrated
 * using two FreeRTOS tasks communicating via a FreeRTOS queue, which teaches
 * the same lock-free ring buffer and IPC latency concepts.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "adc_producer.h"
#include "fir_consumer.h"
#include "shared_mem.h"

/* Shared ring buffer and latency measurement (in normal BSS on single-core) */
RingBuf_t     g_ring;
LatencyTest_t g_latency;

/* Queue used for IPC notification (replaces MU on single-core MCXN236) */
QueueHandle_t g_notify_queue;

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    PRINTF("[MAIN] FATAL: malloc failed\r\n");
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    taskDISABLE_INTERRUPTS();
    PRINTF("[MAIN] FATAL: stack overflow: %s\r\n", pcTaskName);
    for (;;) {}
}

int main(void)
{
    BOARD_InitHardware();

    PRINTF("\r\n=== RTOS Lab 09: Producer-Consumer IPC ===\r\n");

    /* Initialise shared ring buffer */
    g_ring.write_idx      = 0U;
    g_ring.read_idx       = 0U;
    g_ring.missed         = 0U;
    g_latency.send_cycles = 0U;
    g_latency.recv_cycles = 0U;

    /* Create IPC notification queue (depth 8, element = uint32_t write_idx) */
    g_notify_queue = xQueueCreate(8U, sizeof(uint32_t));
    configASSERT(g_notify_queue != NULL);

    /* Create producer (priority 3) and consumer (priority 2) tasks */
    xTaskCreate(vADCProducerTask, "ADCProd", 512, NULL, 3, NULL);
    xTaskCreate(vFIRConsumerTask, "FIRCons", 512, NULL, 2, NULL);

    PRINTF("[MAIN] starting scheduler — free heap: %u bytes\r\n",
           (unsigned)xPortGetFreeHeapSize());

    vTaskStartScheduler();
    for (;;) {}
}
