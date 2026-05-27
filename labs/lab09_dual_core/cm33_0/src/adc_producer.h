/*
 * adc_producer.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 09 — Dual-Core AMP: CM33_0 ADC Producer Task
 *
 * vADCProducerTask simulates ADC sampling every 10 ms. Each sample is written
 * into the shared ring buffer (g_ring in SRAM4) with DMB barriers, then a
 * notification is sent to CM33_1 via the Messaging Unit (MU).
 * ─────────────────────────────────────────────────────────────────────────────
 */
#ifndef ADC_PRODUCER_H
#define ADC_PRODUCER_H

#include "FreeRTOS.h"
#include "task.h"

/*
 * vADCProducerTask
 *
 * FreeRTOS task entry point.  Create with xTaskCreate().
 * Priority: 3 (above default, below ISR).
 * Stack:    512 words minimum.
 */
void vADCProducerTask(void *pvParameters);

#endif /* ADC_PRODUCER_H */
