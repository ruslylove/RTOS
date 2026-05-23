#ifndef OVERFLOW_DEMO_H
#define OVERFLOW_DEMO_H

/*
 * overflow_demo.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 07, Part A — Stack Overflow Induction and Detection
 *
 * vOverflowTask creates a deeply-recursive call that intentionally exhausts
 * its stack allocation (64 words = 256 bytes in Part A Step 2).
 *
 * vApplicationStackOverflowHook is the FreeRTOS callback invoked when
 * configCHECK_FOR_STACK_OVERFLOW >= 1 and an overflow is detected at a
 * context switch.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"

/* Task entry point */
void vOverflowTask(void *pvParameters);

/* FreeRTOS overflow hook — must be defined exactly once in the project */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);

#endif /* OVERFLOW_DEMO_H */
