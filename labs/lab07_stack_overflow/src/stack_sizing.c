/*
 * stack_sizing.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 07, Part B — Stack Right-Sizing with High-Water-Mark
 *
 * vStackReportTask waits 30 seconds (allowing all tasks to exercise their
 * worst-case stack usage), then prints a report showing:
 *   - Words allocated per task
 *   - Words remaining (high-water mark — HWM)
 *   - Minimum words used = allocated - HWM
 *   - Suggested right-sized stack = min_used * 1.20 (20 % headroom)
 *
 * Add or remove handles from the handles[] / names[] arrays to include all
 * tasks you want to analyse.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "stack_sizing.h"
#include "fsl_debug_console.h"

/* Allocated stack depths (words) — must match xTaskCreate() calls in main.c */
#define OVERFLOW_STACK_WORDS  512
#define SENSOR_STACK_WORDS    256
#define LOGGER_STACK_WORDS    256
#define MONITOR_STACK_WORDS   256

/* Headroom factor applied to the observed minimum usage */
#define HEADROOM_PERCENT  20u

/* ── External task handles (declared in main.c) ──────────────────────────── */
extern TaskHandle_t xOverflowHandle;

/*
 * Weak declarations — these resolve to NULL when the pipeline tasks are not
 * created (ENABLE_PIPELINE = 0 in main.c), so the report simply skips them.
 */
extern TaskHandle_t xSensor1Handle  __attribute__((weak));
extern TaskHandle_t xSensor2Handle  __attribute__((weak));
extern TaskHandle_t xLoggerHandle   __attribute__((weak));
extern TaskHandle_t xMonitorHandle  __attribute__((weak));

TaskHandle_t xSensor1Handle = NULL;
TaskHandle_t xSensor2Handle = NULL;
TaskHandle_t xLoggerHandle  = NULL;
TaskHandle_t xMonitorHandle = NULL;

/* ══════════════════════════════════════════════════════════════════════════ */
/* vStackReportTask                                                            */
/* ══════════════════════════════════════════════════════════════════════════ */

void vStackReportTask(void *pvParameters)
{
    (void)pvParameters;

    /* Wait long enough for all tasks to exercise their deepest stack paths */
    vTaskDelay(pdMS_TO_TICKS(30000));

    /*
     * Table of task handles, names, and allocated stack depths.
     * Extend this array with any additional tasks you create.
     */
    struct {
        TaskHandle_t  handle;
        const char   *name;
        uint32_t      allocated_words;
    } tasks[] = {
        { xOverflowHandle, "Overflow", OVERFLOW_STACK_WORDS },
        { xSensor1Handle,  "Sensor1",  SENSOR_STACK_WORDS   },
        { xSensor2Handle,  "Sensor2",  SENSOR_STACK_WORDS   },
        { xLoggerHandle,   "Logger",   LOGGER_STACK_WORDS   },
        { xMonitorHandle,  "Monitor",  MONITOR_STACK_WORDS  },
    };
    const int n_tasks = (int)(sizeof(tasks) / sizeof(tasks[0]));

    PRINTF("\r\n=== Stack High-Water Mark Report (after 30 s) ===\r\n");
    PRINTF("  %-16s  %8s  %8s  %8s  %12s\r\n",
                "Task", "Alloc'd", "HWM rem.", "Min used", "Rgt-sized+20%");
    PRINTF("  ---------------------------------------------------------------\r\n");

    for (int i = 0; i < n_tasks; i++)
    {
        if (tasks[i].handle == NULL)
            continue;   /* task not created in this build */

        UBaseType_t hwm  = uxTaskGetStackHighWaterMark(tasks[i].handle);
        uint32_t    used = tasks[i].allocated_words - (uint32_t)hwm;
        uint32_t    recs = used + (used * HEADROOM_PERCENT / 100u);

        PRINTF("  %-16s  %8lu  %8lu  %8lu  %12lu\r\n",
                    tasks[i].name,
                    tasks[i].allocated_words,
                    (uint32_t)hwm,
                    used,
                    recs);
    }

    PRINTF("  ---------------------------------------------------------------\r\n");
    PRINTF("  Free heap now:         %u bytes\r\n",
                (unsigned)xPortGetFreeHeapSize());
    PRINTF("  Minimum ever free:     %u bytes\r\n",
                (unsigned)xPortGetMinimumEverFreeHeapSize());

    /* This task's job is done */
    vTaskDelete(NULL);
}
