/*
 * adc_task.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 10 — Tickless Idle: ADC Sampling Task
 *
 * vADCTask samples a simulated ADC channel at 2 Hz (500 ms period) using
 * vTaskDelayUntil for deterministic timing.  It prints each reading and a
 * running min/max/mean for analysis of timing jitter introduced (or removed)
 * by the tickless idle mode.
 * ─────────────────────────────────────────────────────────────────────────────
 */
#ifndef ADC_TASK_H
#define ADC_TASK_H

#include "FreeRTOS.h"
#include "task.h"

/*
 * vADCTask
 *
 * Period: 500 ms (configurable via ADC_PERIOD_MS in adc_task.c).
 * Priority: 2.
 * Stack: 256 words minimum.
 */
void vADCTask(void *pvParameters);

#endif /* ADC_TASK_H */
