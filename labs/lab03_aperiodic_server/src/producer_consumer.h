#ifndef PRODUCER_CONSUMER_H
#define PRODUCER_CONSUMER_H

/*
 * producer_consumer.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 03, Part C — Two-Producer, One-Consumer Pipeline
 *
 * Two sensor tasks produce readings at different rates into xPipelineQueue.
 * vLoggerTask consumes the queue and sets event bits in xPipelineEvents.
 * vMonitorTask waits for event group conditions.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include <stdint.h>

/* ── Shared data type ─────────────────────────────────────────────────────── */
typedef struct {
    uint8_t  sensor_id;   /* 1 or 2 */
    int16_t  value;       /* raw sensor value */
    uint32_t timestamp;   /* tick count at production time */
} SensorReading_t;

/* ── Event group bit definitions ─────────────────────────────────────────── */
#define BIT_DATA_READY   (1 << 0)   /* set every 10 items consumed */
#define BIT_ALARM        (1 << 1)   /* set when value > 800        */

/* ── Kernel objects — created in main.c ──────────────────────────────────── */
extern QueueHandle_t      xPipelineQueue;
extern EventGroupHandle_t xPipelineEvents;

/* ── Task entry points ───────────────────────────────────────────────────── */
void vSensor1Task(void *pvParameters);  /* T=100 ms, prio 3 */
void vSensor2Task(void *pvParameters);  /* T=200 ms, prio 2 */
void vLoggerTask(void *pvParameters);   /* prio 1 — consumer */
void vMonitorTask(void *pvParameters);  /* prio 2 — event group waiter */

#endif /* PRODUCER_CONSUMER_H */
