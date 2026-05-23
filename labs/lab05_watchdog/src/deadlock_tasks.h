#ifndef DEADLOCK_TASKS_H
#define DEADLOCK_TASKS_H

/*
 * deadlock_tasks.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 05, Parts A & B — Deadlock Induction and Prevention
 *
 * Two mutexes, two tasks:
 *   vTaskAlpha acquires MutexA then MutexB (Part A)
 *   vTaskBeta  acquires MutexB then MutexA (Part A) — opposite order!
 *
 * Part B fix: vTaskBeta acquires MutexA first (same order as Alpha).
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "semphr.h"

/* Mutex handles — created in main.c */
extern SemaphoreHandle_t xMutexA;
extern SemaphoreHandle_t xMutexB;

/* Lock-order constants for resource ordering (Part B) */
#define LOCK_ORDER_MUTEX_A  1
#define LOCK_ORDER_MUTEX_B  2

/* Task entry points */
void vTaskAlpha(void *pvParameters);  /* priority 2 */
void vTaskBeta(void *pvParameters);   /* priority 2 */

#endif /* DEADLOCK_TASKS_H */
