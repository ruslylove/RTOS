/*
 * sensor_thread.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 11 — Zephyr RTOS: Sensor Thread
 *
 * Zephyr analogue of Lab 03 vSensor1Task.
 * Reads temperature and humidity from the BME280 every 500 ms and posts
 * readings to the pipeline message queue.
 * ─────────────────────────────────────────────────────────────────────────────
 */
#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H

#include <zephyr/kernel.h>

/* Sensor reading passed between threads via the message queue */
typedef struct {
    int32_t temperature_mdegC;  /* millidegrees Celsius (e.g. 23500 = 23.5 °C) */
    int32_t humidity_mpct;      /* milli-percent RH     (e.g. 55000 = 55.0 %)  */
    int32_t pressure_pa;        /* Pascals                                       */
    uint32_t timestamp_ms;      /* k_uptime_get_32() at time of reading          */
    uint32_t sequence;          /* monotonically increasing sample number        */
} SensorReading_t;

/*
 * sensor_thread_fn
 *
 * Zephyr thread entry function.  Passed to K_THREAD_DEFINE or k_thread_create.
 * Period: 500 ms (pdMS_TO_TICKS equivalent: K_MSEC(500)).
 */
void sensor_thread_fn(void *p1, void *p2, void *p3);

/*
 * g_sensor_queue
 *
 * Message queue shared by sensor_thread_fn (producer) and logger_thread_fn
 * (consumer).  Defined in sensor_thread.c; declared here for other modules.
 */
extern struct k_msgq g_sensor_queue;

#endif /* SENSOR_THREAD_H */
