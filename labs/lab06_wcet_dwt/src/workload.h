#ifndef WORKLOAD_H
#define WORKLOAD_H

/*
 * workload.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 06, Part A — WCET measurement workload functions
 *
 * Three functions of increasing complexity for WCET analysis:
 *   bubble_sort : data-dependent WCET (branch-heavy)
 *   fir_filter  : data-independent WCET (no branches in the hot path)
 *   crc32       : data-dependent WCET (branch on each bit)
 *
 * All functions are pure (no side effects beyond their output).
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>

void     bubble_sort(int16_t *arr, uint16_t n);
int32_t  fir_filter(const int16_t *x, const int16_t *h, uint16_t n);
uint32_t crc32(const uint8_t *data, uint32_t len);

/* Measurement task — runs all three workloads, prints results, then deletes */
void vWcetMeasureTask(void *pvParameters);

/* Part C — periodic sort task running in a multi-task context */
void vSortTask(void *pvParameters);

#endif /* WORKLOAD_H */
