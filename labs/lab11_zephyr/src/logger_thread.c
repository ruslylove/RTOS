/*
 * logger_thread.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 11 — Zephyr RTOS: Logger Thread
 *
 * Blocks on g_sensor_queue indefinitely.  For each received SensorReading_t,
 * it formats and logs the values using LOG_INF so they appear on the Zephyr
 * logging backend (UART by default on FRDM-MCXN236).
 *
 * Note: Zephyr's deferred logging runs output in a dedicated thread; printk()
 * is synchronous but has less formatting flexibility.  We use LOG_INF here.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "logger_thread.h"
#include "sensor_thread.h"   /* for SensorReading_t and g_sensor_queue */

LOG_MODULE_REGISTER(logger, LOG_LEVEL_INF);

void logger_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    SensorReading_t reading;

    LOG_INF("Logger thread started");

    for (;;) {
        /* Block until a reading is available */
        k_msgq_get(&g_sensor_queue, &reading, K_FOREVER);

        /* Format: seq, timestamp, temperature, humidity, pressure */
        LOG_INF("[LOG] seq=%6u  t=%5u ms  "
                "temp=%3d.%03d C  "
                "hum=%2d.%03d %%  "
                "pres=%6d Pa",
                reading.sequence,
                reading.timestamp_ms,
                reading.temperature_mdegC / 1000,
                (int)(reading.temperature_mdegC % 1000),
                reading.humidity_mpct / 1000,
                (int)(reading.humidity_mpct % 1000),
                reading.pressure_pa);

        /*
         * TODO (Part B): write readings to a ring buffer in shared memory or
         * a Zephyr FIFO for the monitor thread to analyse.
         */
    }
}
