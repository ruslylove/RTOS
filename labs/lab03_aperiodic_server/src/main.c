/*
 * main.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 03 — Aperiodic Servers & Producer-Consumer Pipeline
 *
 * This file creates all FreeRTOS kernel objects and tasks, then starts the
 * scheduler. Hardware initialisation is done via BOARD_InitHardware() from
 * the NXP MCUXpresso SDK (clocks to 150 MHz, pin-mux, LPUART4 debug console).
 *
 * Parts:
 *   A — Deferred interrupt handler (xAperiodicSem, vAperiodicHandlerTask)
 *   B — Polling Server           (xRequestQueue,  vPollingServerTask)
 *   C — Producer-consumer pipeline (xPipelineQueue, xPipelineEvents)
 *
 * Enable only the tasks relevant to the part you are working on to keep
 * terminal output readable.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"

#include "FreeRTOSConfig.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "periodic_tasks.h"
#include "aperiodic_handler.h"
#include "producer_consumer.h"

/* ── Select which parts to run ───────────────────────────────────────────── */
/* Comment/uncomment to enable the relevant part.                             */
#define ENABLE_PERIODIC_TASKS    1
#define ENABLE_PART_A            1   /* deferred interrupt handler */
/* #define ENABLE_PART_B         1 */   /* polling server (replaces Part A) */
/* #define ENABLE_PART_C         1 */   /* producer-consumer pipeline        */

/* ── Queue depths ─────────────────────────────────────────────────────────── */
#define REQUEST_QUEUE_DEPTH    4
#define PIPELINE_QUEUE_DEPTH   10

/* ── Kernel object handles (global — extern'd in header files) ────────────── */
SemaphoreHandle_t xAperiodicSem   = NULL;
QueueHandle_t     xRequestQueue   = NULL;
QueueHandle_t     xPipelineQueue  = NULL;
EventGroupHandle_t xPipelineEvents = NULL;

/* ── Hardware init prototype (implement in a separate hw.c or inline below) ─ */
static void hw_init(void);

/* ── FreeRTOS hook implementations ─────────────────────────────────────────── */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    PRINTF("[FATAL] heap allocation failed\r\n");
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    taskDISABLE_INTERRUPTS();
    PRINTF("[FATAL] stack overflow in task: %s\r\n", pcTaskName);
    for (;;) {}
}

/* ════════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    hw_init();

    PRINTF("\r\n=== RTOS Lab 03: Aperiodic Servers & Producer-Consumer ===\r\n\r\n");

    /* ── Create kernel objects ──────────────────────────────────────────── */

#ifdef ENABLE_PART_A
    xAperiodicSem = xSemaphoreCreateBinary();
    configASSERT(xAperiodicSem != NULL);
#endif

#ifdef ENABLE_PART_B
    xRequestQueue = xQueueCreate(REQUEST_QUEUE_DEPTH, sizeof(AperiodicRequest_t));
    configASSERT(xRequestQueue != NULL);
#endif

#ifdef ENABLE_PART_C
    xPipelineQueue  = xQueueCreate(PIPELINE_QUEUE_DEPTH, sizeof(SensorReading_t));
    xPipelineEvents = xEventGroupCreate();
    configASSERT(xPipelineQueue  != NULL);
    configASSERT(xPipelineEvents != NULL);
#endif

    /* ── Create tasks ───────────────────────────────────────────────────── */

#ifdef ENABLE_PERIODIC_TASKS
    xTaskCreate(vPeriodic1Task, "Periodic1", 256, NULL, 3, NULL);
    xTaskCreate(vPeriodic2Task, "Periodic2", 256, NULL, 2, NULL);
#endif

#ifdef ENABLE_PART_A
    /* Handler task priority 4 — above all periodic tasks */
    xTaskCreate(vAperiodicHandlerTask, "AperiodicH", 256, NULL, 4, NULL);
#endif

#ifdef ENABLE_PART_B
    /* Polling Server: priority 3, treated as tau with C=Bs, T=Ts */
    xTaskCreate(vPollingServerTask, "PollingServer", 256, NULL, 3, NULL);
#endif

#ifdef ENABLE_PART_C
    xTaskCreate(vSensor1Task, "Sensor1",  256, NULL, 3, NULL);
    xTaskCreate(vSensor2Task, "Sensor2",  256, NULL, 2, NULL);
    xTaskCreate(vLoggerTask,  "Logger",   512, NULL, 1, NULL);
    xTaskCreate(vMonitorTask, "Monitor",  256, NULL, 2, NULL);
#endif

    /* ── Configure GPIO interrupt for user button (P0_23) ──────────────── */
    /* TODO: initialise GPIO0 pin 23 as input with rising-edge interrupt,
     * then call EnableIRQ(GPIO0_IRQn) and set its NVIC priority to
     * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY so that FromISR APIs work.
     *
     * gpio_pin_config_t btn_cfg = { kGPIO_DigitalInput, 0, kGPIO_IntRisingEdge };
     * GPIO_PinInit(GPIO0, 23, &btn_cfg);
     * GPIO_EnableInterrupts(GPIO0, 1u << 23);
     * NVIC_SetPriority(GPIO0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
     * EnableIRQ(GPIO0_IRQn);
     */

    PRINTF("[MAIN] starting scheduler -- free heap: %u bytes\r\n",
           (unsigned)xPortGetFreeHeapSize());

    vTaskStartScheduler();

    /* Should never reach here */
    for (;;) {}
}

/* ── Hardware initialisation stub ─────────────────────────────────────────── */
static void hw_init(void)
{
    /* TODO: Configure the MCXN236 clock to 150 MHz using the PLL.
     * In an MCUXpresso SDK project this is handled by BOARD_BootClockPLL150M().
     * For a bare-metal project, replicate the relevant clock-init sequence from
     * the SDK's clock_config.c.
     *
     * After the clock is configured, call uart_init() to bring up LPUART4.
     */
    BOARD_InitHardware();
}
