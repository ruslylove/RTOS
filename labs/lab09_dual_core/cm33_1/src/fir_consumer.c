/*
 * fir_consumer.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 09, Part C — FIR Consumer Task (CM33_1)
 *
 * The MU ISR wakes vFIRConsumerTask on each incoming MU message from CM33_0.
 * The task then drains the shared ring buffer, applying a 4-tap
 * moving-average filter to each sample.
 *
 * Part B (latency measurement):
 *   At ISR entry, record g_latency.recv_cycles = DWT->CYCCNT before any other
 *   work; then compute latency = recv_cycles - send_cycles after returning.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "fsl_debug_console.h"
#include "fsl_mu.h"
#include "fir_consumer.h"
#include "shared_mem.h"

/* MU channel used for notifications (must match cm33_0) */
#define MU_CHANNEL  0U

/* FIR filter order */
#define FIR_TAPS  4U

/* Handle set at task startup; used by ISR to post notifications */
TaskHandle_t xFIRTaskHandle = NULL;

/* ── MUA_IRQHandler ──────────────────────────────────────────────────────── */
/*
 * Fires when CM33_0 sends a message via MU channel 0.
 * Records the receive timestamp (Part B), clears the interrupt, and posts a
 * task notification to wake vFIRConsumerTask.
 */
void MUA_IRQHandler(void)
{
    /* Part B: record receive timestamp as early as possible */
    /* TODO (Part B): g_latency.recv_cycles = DWT->CYCCNT; __DMB(); */

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Read (and discard) the message to clear the pending flag */
    (void)MU_ReceiveMsgNonBlocking(MUB, MU_CHANNEL);

    /* Wake the consumer task */
    if (xFIRTaskHandle != NULL)
    {
        vTaskNotifyGiveFromISR(xFIRTaskHandle, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* ── 4-tap moving-average FIR ────────────────────────────────────────────── */

static int32_t fir_filter(int16_t new_sample)
{
    static int16_t history[FIR_TAPS] = {0};
    static uint8_t idx = 0;

    history[idx] = new_sample;
    idx = (uint8_t)((idx + 1U) % FIR_TAPS);

    int32_t acc = 0;
    for (uint8_t i = 0; i < FIR_TAPS; i++)
    {
        acc += history[i];
    }
    return acc / (int32_t)FIR_TAPS;
}

/* ════════════════════════════════════════════════════════════════════════════ */
/* vFIRConsumerTask                                                             */
/* ════════════════════════════════════════════════════════════════════════════ */

void vFIRConsumerTask(void *pvParameters)
{
    (void)pvParameters;

    /* Store handle so the ISR can notify us */
    xFIRTaskHandle = xTaskGetCurrentTaskHandle();

    /* Enable MU receive interrupt on channel 0 */
    MU_EnableInterrupts(MUB, kMU_Rx0FullInterruptEnable);
    NVIC_SetPriority(MUA_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1U);
    NVIC_EnableIRQ(MUA_IRQn);

    uint32_t consumed = 0;
    uint32_t report_tick = xTaskGetTickCount();

    for (;;)
    {
        /* Block until the MU ISR posts a notification (or 100 ms timeout) */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));

        /* ── Drain the ring buffer ──────────────────────────────────────── */
        /* DMB: ensure we see the latest write_idx from CM33_0 */
        __DMB();

        while (g_ring.read_idx != g_ring.write_idx)
        {
            int16_t raw    = g_ring.data[g_ring.read_idx];
            int32_t filtered = fir_filter(raw);

            g_ring.read_idx = (g_ring.read_idx + 1U) % RING_BUF_SIZE;
            consumed++;

            /* TODO (Part C extension): forward filtered value to a logger task
             * or accumulate statistics (min/max/mean) for display.           */
            (void)filtered;
        }

        /* Print status every 1 s */
        TickType_t now = xTaskGetTickCount();
        if ((now - report_tick) >= pdMS_TO_TICKS(1000))
        {
            report_tick = now;
            PRINTF("[CM33_1] consumed=%lu, missed=%lu, free heap=%u\r\n",
                   (unsigned long)consumed,
                   (unsigned long)g_ring.missed,
                   (unsigned)xPortGetFreeHeapSize());

            /* Part B: print latency if data is available */
            if (g_latency.recv_cycles != 0U && g_latency.send_cycles != 0U)
            {
                uint32_t latency = g_latency.recv_cycles - g_latency.send_cycles;
                PRINTF("[CM33_1] MU latency: %lu cycles (~%lu ns @ 150 MHz)\r\n",
                       (unsigned long)latency,
                       (unsigned long)(latency * 1000U / 150U));
            }
        }
    }
}
