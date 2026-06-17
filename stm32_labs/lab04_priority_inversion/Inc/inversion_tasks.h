#ifndef INVERSION_TASKS_H
#define INVERSION_TASKS_H

#include "FreeRTOS.h"
#include "semphr.h"

/* Shared resource guard — binary semaphore triggers inversion,
   mutex with priority inheritance resolves it. */
extern SemaphoreHandle_t xSharedResource;

/* Select guard type: define USE_MUTEX to use a priority-inheriting mutex,
   leave undefined to use a plain binary semaphore (shows the inversion). */
/* #define USE_MUTEX */

void vHighPrioTask (void *pvParameters); /* prio 5 — blocked by low holding resource */
void vMedPrioTask  (void *pvParameters); /* prio 3 — causes unbounded blocking */
void vLowPrioTask  (void *pvParameters); /* prio 1 — holds resource, CPU-bound work */

#endif
