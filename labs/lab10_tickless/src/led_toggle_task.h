/*
 * led_toggle_task.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 10 — Tickless Idle: LED Toggle Task
 *
 * vLEDTask toggles the FRDM-MCXN236 RGB LED every 1 second.  It also prints
 * the actual elapsed tick count so any timing drift introduced by tickless
 * idle can be observed and compared to Part A (standard ticking).
 * ─────────────────────────────────────────────────────────────────────────────
 */
#ifndef LED_TOGGLE_TASK_H
#define LED_TOGGLE_TASK_H

#include "FreeRTOS.h"
#include "task.h"

/*
 * vLEDTask
 *
 * Period: 1000 ms (configurable via LED_PERIOD_MS in led_toggle_task.c).
 * Priority: 1 (lower than ADC task).
 * Stack: 256 words minimum.
 */
void vLEDTask(void *pvParameters);

#endif /* LED_TOGGLE_TASK_H */
