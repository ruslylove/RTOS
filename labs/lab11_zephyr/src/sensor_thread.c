/*
 * sensor_thread.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 11 — Zephyr RTOS: Sensor Thread
 *
 * Reads from a BME280 (or simulated fallback) every 500 ms using
 * k_sleep(K_MSEC(500)).  Readings are pushed to g_sensor_queue for
 * logger_thread_fn to consume.
 *
 * Zephyr sensor API reference:
 *   sensor_sample_fetch()  — trigger a new measurement
 *   sensor_channel_get()   — read a specific channel into sensor_value
 *
 * sensor_value to double:  sensor_value_to_double(&val)
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "sensor_thread.h"

LOG_MODULE_REGISTER(sensor, LOG_LEVEL_INF);

/* BME280 device node (defined in app.overlay via alias bme280-sensor) */
#define BME280_NODE DT_ALIAS(bme280_sensor)

/* Message queue: holds up to 8 SensorReading_t items, 4-byte aligned */
K_MSGQ_DEFINE(g_sensor_queue, sizeof(SensorReading_t), 8, 4);

/* Simulated sensor state (fallback when hardware is not present) */
static int32_t s_sim_temp = 23500;   /* start at 23.5 °C */
static int32_t s_sim_hum  = 55000;   /* start at 55.0 % RH */
static int32_t s_sim_pres = 101325;  /* start at 1013.25 hPa in Pa */

static void simulate_sensor(SensorReading_t *out)
{
    /* Slowly drift values to make the output interesting */
    s_sim_temp  = (s_sim_temp  + 10)  % 50000;   /* 0..50 °C  */
    s_sim_hum   = (s_sim_hum   + 50)  % 100000;  /* 0..100 %  */
    s_sim_pres  = 100000 + (s_sim_pres % 3000);   /* ~1000 hPa */

    out->temperature_mdegC = s_sim_temp;
    out->humidity_mpct     = s_sim_hum;
    out->pressure_pa       = s_sim_pres;
}

void sensor_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    const struct device *bme280 = NULL;

#if DT_NODE_HAS_STATUS(BME280_NODE, okay)
    bme280 = DEVICE_DT_GET(BME280_NODE);
    if (!device_is_ready(bme280)) {
        LOG_WRN("BME280 not ready — using simulated values");
        bme280 = NULL;
    } else {
        LOG_INF("BME280 ready");
    }
#else
    LOG_INF("BME280 node not in device tree — using simulated values");
#endif

    uint32_t sequence = 0U;

    for (;;) {
        SensorReading_t reading = {0};
        reading.timestamp_ms = k_uptime_get_32();
        reading.sequence     = sequence++;

        if (bme280 != NULL) {
            /* ── Real BME280 read ──────────────────────────────────────── */
            int rc = sensor_sample_fetch(bme280);
            if (rc < 0) {
                LOG_ERR("sensor_sample_fetch failed: %d", rc);
            } else {
                struct sensor_value val;

                sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP, &val);
                reading.temperature_mdegC = val.val1 * 1000 + val.val2 / 1000;

                sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY, &val);
                reading.humidity_mpct = val.val1 * 1000 + val.val2 / 1000;

                sensor_channel_get(bme280, SENSOR_CHAN_PRESS, &val);
                /* pressure in kPa from driver; convert to Pa */
                reading.pressure_pa = val.val1 * 1000 + val.val2 / 1000;
            }
        } else {
            /* ── Simulated fallback ────────────────────────────────────── */
            simulate_sensor(&reading);
        }

        /* Push to logger queue; if full, drop the oldest (non-blocking put) */
        if (k_msgq_put(&g_sensor_queue, &reading, K_NO_WAIT) != 0) {
            LOG_WRN("sensor queue full, dropping reading #%u", reading.sequence);
            k_msgq_purge(&g_sensor_queue);
            k_msgq_put(&g_sensor_queue, &reading, K_NO_WAIT);
        }

        LOG_DBG("seq=%u temp=%d.%03d C  hum=%d.%03d %%",
                reading.sequence,
                reading.temperature_mdegC / 1000,
                reading.temperature_mdegC % 1000,
                reading.humidity_mpct / 1000,
                reading.humidity_mpct % 1000);

        k_sleep(K_MSEC(500));
    }
}
