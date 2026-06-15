/*
 * fir_consumer.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 09 — Dual-Core AMP: CM33_1 FIR Consumer Task
 *
 * vFIRConsumerTask waits on a task notification posted from the MU ISR.
 * When woken, it drains the shared ring buffer and applies a simple 4-tap
 * moving-average FIR filter to each batch of samples.
 * ─────────────────────────────────────────────────────────────────────────────
 */
#ifndef FIR_CONSUMER_H
#define FIR_CONSUMER_H

#include "FreeRTOS.h"
#include "task.h"

/*
 * vFIRConsumerTask
 *
 * FreeRTOS task entry point.  Create with xTaskCreate().
 * Priority: 3 (same as producer — round-robin when both are ready).
 * Stack:    512 words minimum.
 *
 * The task handle must be stored so the MU ISR can call
 * vTaskNotifyGiveFromISR().
 */
void vFIRConsumerTask(void *pvParameters);

/*
 * xFIRTaskHandle
 *
 * Set by vFIRConsumerTask on entry; used by the MU ISR to post notifications.
 * Declared here so main_cm33_1.c and the MU ISR can reference it.
 */
extern TaskHandle_t xFIRTaskHandle;

/*
 * MUA_IRQHandler
 *
 * Messaging Unit interrupt handler.  Posts a task notification to
 * xFIRTaskHandle when a message arrives on MU channel 0.
 * Defined in fir_consumer.c; the ISR name must match the MCXN236 vector table.
 */
void MUA_IRQHandler(void);

#endif /* FIR_CONSUMER_H */
