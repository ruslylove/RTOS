/*
 * monitor_thread.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 11 — Zephyr RTOS: Monitor Thread
 *
 * Every 5 seconds this thread prints:
 *   1. Stack high-water-mark for each application thread.
 *   2. Current message queue depth.
 *   3. System uptime.
 *
 * Zephyr equivalent of FreeRTOS uxTaskGetStackHighWaterMark is
 * k_thread_stack_space_get() (returns bytes free at the stack's deepest point).
 *
 * Note: CONFIG_THREAD_STACK_INFO=y must be set in prj.conf (already done).
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "monitor_thread.h"
#include "sensor_thread.h"   /* for g_sensor_queue */

LOG_MODULE_REGISTER(monitor, LOG_LEVEL_INF);

/* Thread IDs — set by main() before the threads start */
k_tid_t g_sensor_tid  = NULL;
k_tid_t g_logger_tid  = NULL;
k_tid_t g_monitor_tid = NULL;

/* Helper: safely get stack space or return -1 if not available */
static int get_stack_unused(k_tid_t tid)
{
#ifdef CONFIG_THREAD_STACK_INFO
    size_t unused = 0;
    int rc = k_thread_stack_space_get(tid, &unused);
    return (rc == 0) ? (int)unused : -1;
#else
    (void)tid;
    return -1;
#endif
}

void monitor_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("Monitor thread started");

    for (;;) {
        k_sleep(K_SECONDS(5));

        uint32_t uptime_s = (uint32_t)(k_uptime_get() / 1000U);

        LOG_INF("=== System Monitor (uptime %u s) ===", uptime_s);

        /* ── Stack usage ───────────────────────────────────────────────── */
        LOG_INF("  Thread stack unused bytes:");
        if (g_sensor_tid != NULL) {
            LOG_INF("    sensor:  %d bytes free", get_stack_unused(g_sensor_tid));
        }
        if (g_logger_tid != NULL) {
            LOG_INF("    logger:  %d bytes free", get_stack_unused(g_logger_tid));
        }
        if (g_monitor_tid != NULL) {
            LOG_INF("    monitor: %d bytes free", get_stack_unused(g_monitor_tid));
        }

        /* ── Message queue depth ───────────────────────────────────────── */
        uint32_t queued = k_msgq_num_used_get(&g_sensor_queue);
        uint32_t max_q  = CONFIG_HEAP_MEM_POOL_SIZE; /* rough proxy */
        LOG_INF("  Sensor queue: %u items pending", queued);
        (void)max_q;

        /* ── Heap ─────────────────────────────────────────────────────── */
        /*
         * TODO (optional): use sys_heap_runtime_stats_get() with
         * CONFIG_SYS_HEAP_RUNTIME_STATS=y to print allocated/free bytes.
         */

        LOG_INF("=====================================");
    }
}
