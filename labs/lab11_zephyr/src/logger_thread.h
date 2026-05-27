/*
 * logger_thread.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 11 — Zephyr RTOS: Logger Thread
 *
 * Zephyr analogue of Lab 03 vLoggerTask.
 * Consumes SensorReading_t items from g_sensor_queue and logs them to the
 * console using the Zephyr logging subsystem.
 * ─────────────────────────────────────────────────────────────────────────────
 */
#ifndef LOGGER_THREAD_H
#define LOGGER_THREAD_H

#include <zephyr/kernel.h>

/*
 * logger_thread_fn
 *
 * Blocks on g_sensor_queue (K_FOREVER), formats each reading, and logs it.
 */
void logger_thread_fn(void *p1, void *p2, void *p3);

#endif /* LOGGER_THREAD_H */
