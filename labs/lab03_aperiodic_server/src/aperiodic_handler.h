#ifndef APERIODIC_HANDLER_H
#define APERIODIC_HANDLER_H

/*
 * aperiodic_handler.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 03, Parts A & B
 *
 * Part A — Deferred Interrupt Handler
 *   GPIO0_IRQHandler  : button ISR — gives xAperiodicSem
 *   vAperiodicHandlerTask : high-priority task that handles the event
 *
 * Part B — Polling Server
 *   GPIO0_IRQHandler  : (Part B variant) enqueues request to xRequestQueue
 *   vPollingServerTask : drains xRequestQueue within a time budget each period
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"

/* Kernel objects — created in main.c, used here */
extern SemaphoreHandle_t xAperiodicSem;
extern QueueHandle_t     xRequestQueue;

/* Request type sent through xRequestQueue (Part B) */
typedef struct {
    uint8_t  type;        /* request type identifier */
    uint32_t timestamp;   /* tick count at enqueue time */
} AperiodicRequest_t;

/* Task entry points */
void vAperiodicHandlerTask(void *pvParameters); /* Part A */
void vPollingServerTask(void *pvParameters);    /* Part B */

/* ISR — defined in aperiodic_handler.c */
void GPIO0_IRQHandler(void);

#endif /* APERIODIC_HANDLER_H */
