/*
 * watchdog.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 05, Parts C & D — Hardware and Software Watchdog
 *
 * Requires the NXP MCUXpresso SDK: fsl_wdog32.h / fsl_wdog32.c
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"
#include "watchdog.h"
#include "uart.h"

/* SDK WDOG32 driver — available in MCUXpresso SDK or as a standalone file */
#include "fsl_wdog32.h"

/*
 * Set SIMULATE_HANG=1 to test the watchdog reset path.
 * Build: make CFLAGS_EXTRA="-DSIMULATE_HANG=1"
 */
#ifndef SIMULATE_HANG
#define SIMULATE_HANG 0
#endif

/* ══════════════════════════════════════════════════════════════════════════ */
/* Part C — Hardware WDOG32                                                   */
/* ══════════════════════════════════════════════════════════════════════════ */

/*
 * WDOG_Init_1s
 *
 * Configure WDOG32 with a ~1 second timeout using the LPO clock at 128 Hz.
 * timeoutValue = 0x100 -> 256 LPO ticks -> 256/128 = 2.0 s (see Question C1).
 *
 * TODO: Verify the exact LPO frequency on your board using the reference
 * manual and adjust timeoutValue for a true 1 s timeout.
 */
void WDOG_Init_1s(void)
{
    wdog32_config_t cfg;
    WDOG32_GetDefaultConfig(&cfg);

    /* TODO: tune these fields for the desired timeout */
    cfg.timeoutValue = 0x80U;           /* ~1 s at LPO 128 Hz */
    cfg.enableWdog32 = true;
    cfg.clockSource  = kWDOG32_ClockSourceLpoClk;
    cfg.prescaler    = kWDOG32_ClockPrescalerDivide1;

    WDOG32_Init(WDOG0, &cfg);
    uart_printf("[WDG] WDOG32 initialised — timeout ~1 s\r\n");
}

/*
 * WDOG_Kick
 *
 * Strobe (refresh) the watchdog to prevent a reset.
 * Must be called within every timeout window.
 */
void WDOG_Kick(void)
{
    WDOG32_Refresh(WDOG0);
}

/*
 * vWatchdogTask (Part C)
 *
 * Highest-priority task (priority 4). Kicks the hardware watchdog every
 * 500 ms. When SIMULATE_HANG is set, deliberately misses the kick to
 * trigger a reset and observe the boot message.
 */
void vWatchdogTask(void *pvParameters)
{
    (void)pvParameters;

    WDOG_Init_1s();

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(500));

#if SIMULATE_HANG
        /* Miss the watchdog deadline — reset expected after ~1 s */
        uart_printf("[WDG] simulating hang — NOT kicking!\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
#else
        WDOG_Kick();
        uart_printf("[WDG] kicked  t=%lu ms\r\n", (uint32_t)xTaskGetTickCount());
#endif
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Part D — Software Watchdog Monitor                                         */
/* ══════════════════════════════════════════════════════════════════════════ */

/* Per-worker alive flags — written by each worker, read by the monitor */
static volatile uint8_t worker_alive[NUM_WORKERS];

/*
 * wdog_checkin
 *
 * Called by each worker task at the end of each cycle to signal liveness.
 * Uses a simple flag rather than a task notification for demonstration
 * purposes (see Question D2 for pros/cons).
 */
void wdog_checkin(uint8_t worker_id)
{
    if (worker_id < NUM_WORKERS)
    {
        worker_alive[worker_id] = 1;
    }
}

/*
 * vWatchdogMonitor (Part D)
 *
 * Runs at priority 4. Every 500 ms it checks that every worker has set
 * its alive flag, then resets all flags. Only kicks the hardware watchdog
 * when ALL workers are confirmed alive.
 */
void vWatchdogMonitor(void *pvParameters)
{
    (void)pvParameters;

    WDOG_Init_1s();

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(500));

        bool all_ok = true;

        for (int i = 0; i < NUM_WORKERS; i++)
        {
            if (!worker_alive[i])
            {
                uart_printf("[WDG] worker %d silent -- NOT kicking!\r\n", i);
                all_ok = false;
            }
            worker_alive[i] = 0;   /* reset for next window */
        }

        if (all_ok)
        {
            WDOG_Kick();
            uart_printf("[WDG] all workers OK -- kicked  t=%lu ms\r\n",
                        (uint32_t)xTaskGetTickCount());
        }
        /* If not all OK, watchdog expires on the next timeout -> system reset */
    }
}

/*
 * vWorkerTask (Part D)
 *
 * Generic worker task. The worker ID is passed as the pvParameters argument
 * cast to uint8_t. Each worker checks in every 200 ms.
 *
 * Experiment: set HANG_WORKER_ID to the ID of a worker you want to hang,
 * and set HANG_AFTER_MS to the time after which it stops checking in.
 */
#define HANG_WORKER_ID   1
#define HANG_AFTER_MS    5000

void vWorkerTask(void *pvParameters)
{
    uint8_t id = (uint8_t)(uintptr_t)pvParameters;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(200));

        /* TODO (experiment): simulate a hang for worker 1 after 5 s */
        if (id == HANG_WORKER_ID &&
            xTaskGetTickCount() > pdMS_TO_TICKS(HANG_AFTER_MS))
        {
            uart_printf("[W%d] hanging -- no more check-ins!\r\n", id);
            for (;;) {}   /* infinite loop — watchdog should fire */
        }

        wdog_checkin(id);
        uart_printf("[W%d] working  t=%lu ms\r\n", id, (uint32_t)xTaskGetTickCount());
    }
}
