#ifndef APERIODIC_HANDLER_H
#define APERIODIC_HANDLER_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"

typedef struct {
    uint32_t event_id;
    uint32_t timestamp_ms;
} AperiodicRequest_t;

extern SemaphoreHandle_t xAperiodicSem;   /* Part A: deferred handler */
extern QueueHandle_t     xRequestQueue;   /* Part B: polling server   */

/* Part A — deferred interrupt handler */
void vAperiodicHandlerTask(void *pvParameters);

/* Part B — polling server */
void vPollingServerTask(void *pvParameters);

/* Simulates a button IRQ — call from a software timer or task */
void vSimulateAperiodicEvent(void);

#endif
