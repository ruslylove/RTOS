#ifndef STACK_SIZING_H
#define STACK_SIZING_H

/*
 * stack_sizing.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 07, Part B — Stack Right-Sizing with High-Water-Mark
 *
 * vStackReportTask waits 30 seconds, then prints the high-water mark for each
 * registered task handle.  Add all task handles you want to inspect to the
 * handles[] array inside vStackReportTask.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"

/* Task entry point */
void vStackReportTask(void *pvParameters);

#endif /* STACK_SIZING_H */
