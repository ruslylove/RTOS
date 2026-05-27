/*
 * adc_task.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 10 — Tickless Idle: ADC Sampling Task
 *
 * Uses vTaskDelayUntil for a strict 500 ms period.  With tickless idle enabled
 * the scheduler will suppress ticks between wakeups; timing accuracy across
 * the sleep boundary is the key measurement for Part B.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "fsl_debug_console.h"
#include "adc_task.h"

/* Sampling period */
#define ADC_PERIOD_MS  500U

/* Simulated ADC: sawtooth waveform for easy visual verification */
static int16_t simulate_adc(void)
{
    static int16_t value = 0;
    value = (int16_t)((value + 64) & 0x0FFF);  /* 12-bit sawtooth */
    return value;
}

void vADCTask(void *pvParameters)
{
    (void)pvParameters;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(ADC_PERIOD_MS);

    int32_t  min_val  =  32767;
    int32_t  max_val  = -32768;
    int64_t  sum      = 0;
    uint32_t count    = 0;

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        int16_t sample = simulate_adc();
        count++;
        sum += sample;
        if (sample < min_val) min_val = sample;
        if (sample > max_val) max_val = sample;

        PRINTF("[ADC] t=%5lu ms  sample=%4d  min=%4ld  max=%4ld  mean=%4ld\r\n",
               (unsigned long)xTaskGetTickCount(),
               (int)sample,
               (long)min_val,
               (long)max_val,
               (long)(sum / (int64_t)count));
    }
}
