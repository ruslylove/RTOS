#ifndef STATIC_ALLOC_H
#define STATIC_ALLOC_H

/*
 * static_alloc.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 07, Part D — Static Task Allocation
 *
 * Demonstrates converting vLoggerTask from dynamic heap allocation to static
 * allocation. The TCB and stack are placed in .bss (static storage) rather
 * than being allocated at runtime from ucHeap.
 *
 * Requires in FreeRTOSConfig.h:
 *   configSUPPORT_STATIC_ALLOCATION  = 1
 *   configSUPPORT_DYNAMIC_ALLOCATION = 1   (keep both; only Logger goes static)
 *
 * Also requires vApplicationGetIdleTaskMemory() to be defined — see
 * static_alloc.c.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"

/* Stack depth for the statically-allocated Logger task (words).
 * Use the right-sized value from Part B here. */
#define STATIC_LOGGER_STACK_WORDS  256

/* Create the static Logger task; returns the handle */
TaskHandle_t Static_CreateLoggerTask(void);

/* Required FreeRTOS callback — must be defined when static allocation is on */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t  **ppxIdleTaskStackBuffer,
                                    uint32_t      *pulIdleTaskStackSize);

#endif /* STATIC_ALLOC_H */
