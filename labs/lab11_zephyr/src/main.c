/*
 * main.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 11 — Zephyr RTOS: Sensor + Logger + Monitor Pipeline
 *
 * This is the Zephyr equivalent of the Lab 03 FreeRTOS pipeline.
 *
 * FreeRTOS → Zephyr API mapping used in this lab:
 *   xTaskCreate()              → k_thread_create()
 *   vTaskDelay(pdMS_TO_TICKS)  → k_sleep(K_MSEC())
 *   xQueueCreate / xQueueSend  → K_MSGQ_DEFINE / k_msgq_put
 *   xQueueReceive              → k_msgq_get
 *   vTaskStartScheduler()      → (implicit — Zephyr starts automatically)
 *   PRINTF()                   → LOG_INF() or printk()
 *   configTOTAL_HEAP_SIZE      → CONFIG_HEAP_MEM_POOL_SIZE in prj.conf
 *   uxTaskGetStackHighWaterMark → k_thread_stack_space_get()
 *
 * Build: west build -b frdm_mcxn236 .
 * Flash: west flash
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "sensor_thread.h"
#include "logger_thread.h"
#include "monitor_thread.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* ── Thread stack areas ─────────────────────────────────────────────────── */
#define SENSOR_STACK_SIZE   1024
#define LOGGER_STACK_SIZE   1024
#define MONITOR_STACK_SIZE  1024

K_THREAD_STACK_DEFINE(sensor_stack,  SENSOR_STACK_SIZE);
K_THREAD_STACK_DEFINE(logger_stack,  LOGGER_STACK_SIZE);
K_THREAD_STACK_DEFINE(monitor_stack, MONITOR_STACK_SIZE);

/* ── Thread control blocks ──────────────────────────────────────────────── */
static struct k_thread sensor_tcb;
static struct k_thread logger_tcb;
static struct k_thread monitor_tcb;

/* ════════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    LOG_INF("\r\n=== RTOS Lab 11: Zephyr Sensor Pipeline (FRDM-MCXN236) ===");

    /*
     * Create sensor thread (priority 5 — higher number = lower Zephyr priority;
     * Zephyr uses 0 = highest, unlike FreeRTOS which uses higher number =
     * higher priority).
     *
     * Zephyr cooperative threads have negative priorities; preemptive threads
     * have priorities 0..CONFIG_NUM_PREEMPT_PRIORITIES-1.
     */
    g_sensor_tid = k_thread_create(
        &sensor_tcb,
        sensor_stack,  K_THREAD_STACK_SIZEOF(sensor_stack),
        sensor_thread_fn,
        NULL, NULL, NULL,
        5,             /* priority */
        0,             /* options  */
        K_NO_WAIT);    /* delay    */
    k_thread_name_set(g_sensor_tid, "sensor");

    /* Create logger thread (lower priority than sensor) */
    g_logger_tid = k_thread_create(
        &logger_tcb,
        logger_stack,  K_THREAD_STACK_SIZEOF(logger_stack),
        logger_thread_fn,
        NULL, NULL, NULL,
        6,
        0,
        K_NO_WAIT);
    k_thread_name_set(g_logger_tid, "logger");

    /* Create monitor thread (lowest priority — background reporting) */
    g_monitor_tid = k_thread_create(
        &monitor_tcb,
        monitor_stack, K_THREAD_STACK_SIZEOF(monitor_stack),
        monitor_thread_fn,
        NULL, NULL, NULL,
        7,
        0,
        K_NO_WAIT);
    k_thread_name_set(g_monitor_tid, "monitor");

    LOG_INF("All threads created. Zephyr scheduler running.");

    /*
     * In Zephyr, main() itself runs as a thread.  Return here allows the
     * kernel to idle; all work happens in the three threads above.
     *
     * Alternatively, use main as an additional control thread by looping.
     */
    return 0;
}
