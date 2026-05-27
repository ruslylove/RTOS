/*
 * monitor_thread.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 11 — Zephyr RTOS: Monitor Thread
 *
 * Zephyr analogue of Lab 03 vMonitorTask.
 * Wakes every 5 seconds and prints a summary of:
 *   - Thread stack usage (via k_thread_stack_space_get or CONFIG_THREAD_STACK_INFO)
 *   - Heap statistics (via sys_heap_runtime_stats_get)
 *   - Number of sensor readings logged since last report
 * ─────────────────────────────────────────────────────────────────────────────
 */
#ifndef MONITOR_THREAD_H
#define MONITOR_THREAD_H

#include <zephyr/kernel.h>

/*
 * monitor_thread_fn
 *
 * Wakes every 5 s via k_sleep(K_SECONDS(5)).
 */
void monitor_thread_fn(void *p1, void *p2, void *p3);

/* Thread IDs set by main.c so the monitor can query stack usage */
extern k_tid_t g_sensor_tid;
extern k_tid_t g_logger_tid;
extern k_tid_t g_monitor_tid;

#endif /* MONITOR_THREAD_H */
