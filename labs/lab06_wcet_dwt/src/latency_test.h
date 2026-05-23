#ifndef LATENCY_TEST_H
#define LATENCY_TEST_H

/*
 * latency_test.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 06, Part B — GPIO ISR Interrupt Latency Measurement
 *
 * Hardware connections:
 *   P1_0  output — driven by vGpioToggleTask to generate the test signal
 *   P1_1  input  — rising-edge interrupt source (wire to P1_0)
 *   P1_2  output — toggled immediately in the ISR (probe with oscilloscope)
 *
 * Latency is measured from P1_1 assertion to the first instruction in the ISR.
 * Option A: oscilloscope on P1_1 (CH1) and P1_2 (CH2)
 * Option C: DWT inside the ISR (see latency_test.c)
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/* Depth of the DWT-based latency sample buffer */
#define LATENCY_BUF_SIZE  64

/* Filled by the ISR — read by vLatencyReportTask */
extern volatile uint32_t latency_buffer[LATENCY_BUF_SIZE];
extern volatile uint32_t latency_idx;

/* Shared: the task writes the DWT time just before toggling P1_0 */
extern volatile uint32_t last_gpio_toggle;

/* Task entry points */
void vGpioToggleTask(void *pvParameters);    /* generates the test signal */
void vCriticalSectionTask(void *pvParameters); /* holds a critical section */
void vLatencyReportTask(void *pvParameters); /* prints collected samples */

/* ISR — must be defined in latency_test.c (linked to GPIO1_IRQn) */
void GPIO1_IRQHandler(void);

/* One-time GPIO and NVIC setup — call from main() before scheduler */
void latency_test_init(void);

#endif /* LATENCY_TEST_H */
