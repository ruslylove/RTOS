/*
 * aperiodic_handler.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 03 — Parts A & B
 *
 * Compile-time switch:
 *   LAB_PART = DEFERRED_ISR   -> Part A (semaphore, default)
 *   LAB_PART = POLLING_SERVER -> Part B (queue)
 *
 * In main.c, uncomment the desired define before including this file, or
 * pass it on the compiler command line:
 *   make CFLAGS_EXTRA="-DLAB_PART=POLLING_SERVER"
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "aperiodic_handler.h"
#include "fsl_debug_console.h"

/* Cortex-M33 CMSIS GPIO driver (from MCUXpresso SDK) */
#include "fsl_gpio.h"

/* ── Part selection ──────────────────────────────────────────────────────────
 * Define LAB_PART to one of these values:
 */
#define DEFERRED_ISR   1
#define POLLING_SERVER 2

#ifndef LAB_PART
#define LAB_PART DEFERRED_ISR   /* default: Part A */
#endif

/* ─────────────────────────────────────────────────────────────────────────── */
/* Polling Server configuration (Part B)                                       */
/* ─────────────────────────────────────────────────────────────────────────── */
#define POLLING_SERVER_BUDGET_MS   10   /* max service work per activation */
#define POLLING_SERVER_PERIOD_MS   50   /* server period (Ts)              */

/* ══════════════════════════════════════════════════════════════════════════ */
/* GPIO0 ISR — user button on P0_23                                          */
/* ══════════════════════════════════════════════════════════════════════════ */

void GPIO0_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Clear the interrupt flag for pin 23 */
    GPIO_ClearPinsInterruptFlags(GPIO0, 1u << 23);

#if LAB_PART == DEFERRED_ISR
    /* Part A: signal the handler task via binary semaphore */
    xSemaphoreGiveFromISR(xAperiodicSem, &xHigherPriorityTaskWoken);

#elif LAB_PART == POLLING_SERVER
    /* Part B: enqueue a request for the Polling Server */
    AperiodicRequest_t req = {
        .type      = 1,
        .timestamp = xTaskGetTickCountFromISR()
    };
    xQueueSendFromISR(xRequestQueue, &req, &xHigherPriorityTaskWoken);
#endif

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Part A — Deferred Interrupt Handler Task                                   */
/* ══════════════════════════════════════════════════════════════════════════ */

/*
 * vAperiodicHandlerTask
 *
 * Priority: 4 (above all periodic tasks so it can pre-empt them immediately).
 * Blocks indefinitely on xAperiodicSem; wakes only when the ISR gives it.
 *
 * TODO: implement the handler body below.
 */
void vAperiodicHandlerTask(void *pvParameters)
{
    (void)pvParameters;

    uint32_t count = 0;

    for (;;)
    {
        /* TODO: Block until the ISR signals us */
        xSemaphoreTake(xAperiodicSem, portMAX_DELAY);

        count++;

        /* TODO: Perform the aperiodic work here.
         * Keep execution time short relative to the Polling Server budget.
         * Example: read a GPIO state, update a flag, log to UART. */
        PRINTF("[ASYNC] button press #%u -- handled in task context  t=%u ms\r\n",
                    count, (uint32_t)xTaskGetTickCount());
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Part B — Polling Server Task                                               */
/* ══════════════════════════════════════════════════════════════════════════ */

/*
 * vPollingServerTask
 *
 * Priority: 3 — treated as a periodic task with C = Bs, T = Ts for
 * schedulability analysis. See utilisation note in FreeRTOSConfig.h.
 *
 * TODO: implement the budget drain loop below.
 */
void vPollingServerTask(void *pvParameters)
{
    (void)pvParameters;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        /* Wait until next server period */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(POLLING_SERVER_PERIOD_MS));

        TickType_t budget_start = xTaskGetTickCount();
        AperiodicRequest_t req;

        /* TODO: Drain the request queue within the budget window.
         * Stop when the queue is empty OR the budget is exhausted. */
        while ((xTaskGetTickCount() - budget_start) <
               pdMS_TO_TICKS(POLLING_SERVER_BUDGET_MS))
        {
            if (xQueueReceive(xRequestQueue, &req, 0) != pdTRUE)
            {
                break; /* queue empty — conserve remaining budget */
            }

            /* TODO: Process the request.
             * Log the arrival timestamp and service timestamp. */
            PRINTF("[PS] served request type=%d  arrived=%u ms  served=%u ms\r\n",
                        req.type, req.timestamp, (uint32_t)xTaskGetTickCount());
        }
    }
}
