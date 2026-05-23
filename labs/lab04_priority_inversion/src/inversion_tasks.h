#ifndef INVERSION_TASKS_H
#define INVERSION_TASKS_H

/*
 * inversion_tasks.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 04 — Priority Inversion: See It, Fix It
 *
 * Three tasks share one resource (xSharedResource):
 *
 *   tH (priority 3, period 200 ms) — high-priority, contends for resource
 *   tM (priority 2, period 150 ms) — medium-priority, no resource (causes inversion)
 *   tL (priority 1, period 300 ms) — low-priority, holds resource for 40 ms
 *
 * Part A: xSharedResource = binary semaphore  -> inversion occurs
 * Part C: xSharedResource = mutex             -> PIP prevents inversion
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "semphr.h"

/* Shared resource handle — created in main.c */
extern SemaphoreHandle_t xSharedResource;

/* Optional second resource for Experiment D1 */
extern SemaphoreHandle_t xResource2;

/* Task entry points */
void vTaskH(void *pvParameters);   /* high   priority 3 */
void vTaskM(void *pvParameters);   /* medium priority 2 */
void vTaskL(void *pvParameters);   /* low    priority 1 */

#endif /* INVERSION_TASKS_H */
