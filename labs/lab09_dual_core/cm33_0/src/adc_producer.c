/*
 * adc_producer.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 09, Part C — ADC Producer Task (CM33_0)
 *
 * Simulates ADC sampling at 100 Hz (10 ms period) using vTaskDelayUntil.
 * Each sample is written into the lock-free ring buffer in SRAM4, guarded
 * by DMB barriers to ensure store ordering across the AHB bus matrix.
 * A Messaging Unit (MU) message then wakes the consumer on CM33_1.
 *
 * Part B (latency measurement):
 *   Before sending the MU message, record g_latency.send_cycles = DWT->CYCCNT.
 *   CM33_1 records recv_cycles at ISR entry; the difference is the MU latency.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "fsl_debug_console.h"
#include "fsl_mu.h"
#include "adc_producer.h"
#include "shared_mem.h"

/* MU channel used for ADC sample notifications (channel 0) */
#define MU_CHANNEL  0U

/* Simulated ADC: simple linear congruential generator for demo purposes */
static int16_t simulate_adc_sample(void)
{
    static uint32_t lfsr = 0xACE1U;
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400U);
    return (int16_t)(lfsr & 0x0FFF);  /* 12-bit ADC value */
}

/* ════════════════════════════════════════════════════════════════════════════ */
/* vADCProducerTask                                                             */
/* ════════════════════════════════════════════════════════════════════════════ */

void vADCProducerTask(void *pvParameters)
{
    (void)pvParameters;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(10);  /* 100 Hz */

    uint32_t sample_count = 0;

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        /* ── Produce one ADC sample ──────────────────────────────────────── */
        int16_t sample = simulate_adc_sample();

        uint32_t next = (g_ring.write_idx + 1U) % RING_BUF_SIZE;

        if (next == g_ring.read_idx)
        {
            /* Ring buffer full — record dropped sample */
            g_ring.missed++;
            PRINTF("[ADC] buffer full, dropped sample %lu\r\n",
                   (unsigned long)sample_count);
            continue;
        }

        /* Write sample into ring buffer */
        g_ring.data[g_ring.write_idx] = sample;

        /* DMB: ensure data write is visible before updating write_idx */
        __DMB();

        g_ring.write_idx = next;

        /* DMB: ensure write_idx update is visible before MU send */
        __DMB();

        /* ── Part B: record send timestamp for latency measurement ──────── */
        /* TODO (Part B): g_latency.send_cycles = DWT->CYCCNT; __DMB(); */

        /* Notify CM33_1 via Messaging Unit (non-blocking) */
        MU_TrySendMsg(MUA, MU_CHANNEL, (uint32_t)g_ring.write_idx);

        sample_count++;

        /* Print status every 100 samples (every 1 s) */
        if ((sample_count % 100U) == 0U)
        {
            PRINTF("[CM33_0] produced %lu samples, missed=%lu, free heap=%u\r\n",
                   (unsigned long)sample_count,
                   (unsigned long)g_ring.missed,
                   (unsigned)xPortGetFreeHeapSize());
        }
    }
}
