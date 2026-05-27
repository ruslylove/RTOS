/*
 * workload.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 06, Part A — WCET measurement workload functions
 *
 * These are complete, correct implementations — not stubs.
 * Students measure their execution time; they do not modify the functions
 * themselves (modifying the function changes the WCET being measured).
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "workload.h"
#include "dwt.h"
#include "fsl_debug_console.h"

/* ── Test parameters ─────────────────────────────────────────────────────── */
#define NUM_RUNS     1000
#define ARRAY_SIZE   100

/* ══════════════════════════════════════════════════════════════════════════ */
/* Workload function 1 — bubble_sort                                          */
/* WCET is data-dependent: worst case is reverse-sorted input.               */
/* ══════════════════════════════════════════════════════════════════════════ */

void bubble_sort(int16_t *arr, uint16_t n)
{
    for (uint16_t i = 0; i < n - 1u; i++)
    {
        for (uint16_t j = 0; j < n - i - 1u; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                int16_t tmp  = arr[j];
                arr[j]       = arr[j + 1];
                arr[j + 1]   = tmp;
            }
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Workload function 2 — fir_filter                                           */
/* 16-tap FIR: branch-free, data-independent WCET.                           */
/* ══════════════════════════════════════════════════════════════════════════ */

int32_t fir_filter(const int16_t *x, const int16_t *h, uint16_t n)
{
    int32_t acc = 0;
    for (uint16_t i = 0; i < n; i++)
    {
        acc += (int32_t)x[i] * (int32_t)h[i];
    }
    return acc >> 15;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Workload function 3 — crc32                                                */
/* Bit-by-bit CRC32: data-dependent WCET via the branch in the inner loop.  */
/* ══════════════════════════════════════════════════════════════════════════ */

uint32_t crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    while (len--)
    {
        crc ^= (uint32_t)*data++;
        for (int k = 0; k < 8; k++)
        {
            crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1U));
        }
    }
    return ~crc;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Part A — WCET Measurement Task                                             */
/* Runs NUM_RUNS iterations of each workload, reports best/avg/worst.        */
/* ══════════════════════════════════════════════════════════════════════════ */

void vWcetMeasureTask(void *pvParameters)
{
    (void)pvParameters;

    static int16_t arr[ARRAY_SIZE];
    static int16_t sorted[ARRAY_SIZE];
    static int16_t reverse_sorted[ARRAY_SIZE];
    static int16_t random_arr[ARRAY_SIZE];
    static int16_t fir_h[16] = {1, 2, 3, 4, 5, 6, 7, 8, 8, 7, 6, 5, 4, 3, 2, 1};

    uint32_t best, worst, total, cycles;

    /* ── Prepare test vectors ─────────────────────────────────────────── */
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        sorted[i]       = (int16_t)i;
        reverse_sorted[i] = (int16_t)(ARRAY_SIZE - 1 - i);
        random_arr[i]   = (int16_t)((i * 37 + 13) % ARRAY_SIZE);
    }

    /* ── bubble_sort WCET ─────────────────────────────────────────────── */
    PRINTF("\r\n=== bubble_sort (%d elements) ===\r\n", ARRAY_SIZE);

    /* Best case: already sorted */
    memcpy(arr, sorted, sizeof(sorted));
    DWT_START();
    bubble_sort(arr, ARRAY_SIZE);
    DWT_STOP();
    PRINTF("  best case (sorted input):       %lu cycles  (%.2f us)\r\n",
                _dwt_elapsed, DWT_CYCLES_TO_US(_dwt_elapsed));

    /* Worst case: reverse sorted — run NUM_RUNS times */
    best = UINT32_MAX; worst = 0; total = 0;
    for (int r = 0; r < NUM_RUNS; r++)
    {
        memcpy(arr, reverse_sorted, sizeof(reverse_sorted));
        DWT_START();
        bubble_sort(arr, ARRAY_SIZE);
        DWT_STOP();
        cycles = _dwt_elapsed;
        if (cycles < best)  best  = cycles;
        if (cycles > worst) worst = cycles;
        total += cycles;
    }
    PRINTF("  reverse-sorted (%d runs):  best=%lu  worst=%lu  avg=%lu  cycles\r\n",
                NUM_RUNS, best, worst, total / NUM_RUNS);
    PRINTF("  WCET + 20%% margin:              %lu cycles  (%.2f us)\r\n",
                (uint32_t)(worst * 1.2f), worst * 1.2f / 150.0f);

    /* ── fir_filter WCET ──────────────────────────────────────────────── */
    PRINTF("\r\n=== fir_filter (16 taps) ===\r\n");
    best = UINT32_MAX; worst = 0; total = 0;
    for (int r = 0; r < NUM_RUNS; r++)
    {
        DWT_START();
        volatile int32_t result = fir_filter(random_arr, fir_h, 16);
        (void)result;
        DWT_STOP();
        cycles = _dwt_elapsed;
        if (cycles < best)  best  = cycles;
        if (cycles > worst) worst = cycles;
        total += cycles;
    }
    PRINTF("  best=%lu  worst=%lu  avg=%lu  cycles\r\n",
                best, worst, total / NUM_RUNS);
    PRINTF("  WCET + 20%% margin:              %lu cycles  (%.2f us)\r\n",
                (uint32_t)(worst * 1.2f), worst * 1.2f / 150.0f);

    /* ── crc32 WCET ───────────────────────────────────────────────────── */
    PRINTF("\r\n=== crc32 (%d bytes) ===\r\n", ARRAY_SIZE);
    best = UINT32_MAX; worst = 0; total = 0;
    for (int r = 0; r < NUM_RUNS; r++)
    {
        DWT_START();
        volatile uint32_t crc_val = crc32((const uint8_t *)random_arr,
                                          (uint32_t)ARRAY_SIZE * 2u);
        (void)crc_val;
        DWT_STOP();
        cycles = _dwt_elapsed;
        if (cycles < best)  best  = cycles;
        if (cycles > worst) worst = cycles;
        total += cycles;
    }
    PRINTF("  best=%lu  worst=%lu  avg=%lu  cycles\r\n",
                best, worst, total / NUM_RUNS);
    PRINTF("  WCET + 20%% margin:              %lu cycles  (%.2f us)\r\n",
                (uint32_t)(worst * 1.2f), worst * 1.2f / 150.0f);

    PRINTF("\r\nMeasurement complete.\r\n");

    /* Self-delete — measurement is a one-shot task */
    vTaskDelete(NULL);
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Part C — Periodic Sort Task in a Multi-Task Context                        */
/* ══════════════════════════════════════════════════════════════════════════ */

void vSortTask(void *pvParameters)
{
    (void)pvParameters;

    static int16_t arr[ARRAY_SIZE];
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));

        /* Prepare pseudo-random input each period */
        for (int i = 0; i < ARRAY_SIZE; i++)
        {
            arr[i] = (int16_t)(rand() % 1000);
        }

        DWT_START();
        bubble_sort(arr, ARRAY_SIZE);
        DWT_STOP();

        PRINTF("[SORT] %lu cycles  (%.2f us)\r\n",
                    _dwt_elapsed, DWT_CYCLES_TO_US(_dwt_elapsed));
    }
}
