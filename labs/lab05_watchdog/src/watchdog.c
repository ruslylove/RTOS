/*
 * watchdog.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 05, Parts C & D — Hardware and Software Watchdog
 *
 * Uses NXP MCUXpresso SDK: fsl_wwdt.h / fsl_wwdt.c
 * MCXN236 has WWDT0/WWDT1 (Window Watchdog Timer), not WDOG32.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"
#include "watchdog.h"
#include "fsl_debug_console.h"
#include "fsl_wwdt.h"
#include "fsl_clock.h"

/*
 * Set SIMULATE_HANG=1 to test the watchdog reset path.
 * Build: make CFLAGS_EXTRA="-DSIMULATE_HANG=1"
 */
#ifndef SIMULATE_HANG
#define SIMULATE_HANG 0
#endif

/* ══════════════════════════════════════════════════════════════════════════ */
/* Part C — Hardware WWDT                                                     */
/* ══════════════════════════════════════════════════════════════════════════ */

/*
 * WDOG_Init_1s
 *
 * Configure WWDT0 with a ~1 second timeout.
 * WWDT clock on MCXN236: watchdog oscillator, queried via CLOCK_GetWdtClkFreq(0).
 * timeoutValue = clockFreq gives ~1 s timeout.
 */
void WDOG_Init_1s(void)
{
    CLOCK_EnableClock(kCLOCK_Wwdt0);

    uint32_t wdtFreq = CLOCK_GetWdtClkFreq(0U);
    if (wdtFreq == 0U) {
        wdtFreq = 1000000U;   /* fallback: assume 1 MHz */
    }

    wwdt_config_t cfg;
    WWDT_GetDefaultConfig(&cfg);

    cfg.enableWwdt            = true;
    cfg.enableWatchdogReset   = true;
    cfg.enableWatchdogProtect = false;
    cfg.windowValue           = 0xFFFFFFU;  /* no window restriction */
    cfg.timeoutValue          = wdtFreq;    /* ~1 s */
    cfg.warningValue          = 0U;
    cfg.clockFreq_Hz          = wdtFreq;

    WWDT_Init(WWDT0, &cfg);
    PRINTF("[WDG] WWDT0 initialised — timeout ~1 s (clk=%lu Hz)\r\n",
           (unsigned long)wdtFreq);
}

/*
 * WDOG_Kick — refresh the WWDT to prevent a reset.
 */
void WDOG_Kick(void)
{
    WWDT_Refresh(WWDT0);
}

/*
 * vWatchdogTask (Part C)
 *
 * Highest-priority task (priority 4). Kicks the hardware watchdog every
 * 500 ms. When SIMULATE_HANG is set, deliberately misses the kick to
 * trigger a reset.
 */
void vWatchdogTask(void *pvParameters)
{
    (void)pvParameters;

    WDOG_Init_1s();

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(500));

#if SIMULATE_HANG
        PRINTF("[WDG] simulating hang — NOT kicking!\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
#else
        WDOG_Kick();
        PRINTF("[WDG] kicked  t=%lu ms\r\n", (unsigned long)xTaskGetTickCount());
#endif
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Part D — Software Watchdog Monitor                                         */
/* ══════════════════════════════════════════════════════════════════════════ */

static volatile uint8_t worker_alive[NUM_WORKERS];

void wdog_checkin(uint8_t worker_id)
{
    if (worker_id < NUM_WORKERS)
        worker_alive[worker_id] = 1;
}

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
                PRINTF("[WDG] worker %d silent -- NOT kicking!\r\n", i);
                all_ok = false;
            }
            worker_alive[i] = 0;
        }

        if (all_ok)
        {
            WDOG_Kick();
            PRINTF("[WDG] all workers OK -- kicked  t=%lu ms\r\n",
                   (unsigned long)xTaskGetTickCount());
        }
    }
}

#define HANG_WORKER_ID   1
#define HANG_AFTER_MS    5000

void vWorkerTask(void *pvParameters)
{
    uint8_t id = (uint8_t)(uintptr_t)pvParameters;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(200));

        if (id == HANG_WORKER_ID &&
            xTaskGetTickCount() > pdMS_TO_TICKS(HANG_AFTER_MS))
        {
            PRINTF("[W%d] hanging -- no more check-ins!\r\n", id);
            for (;;) {}
        }

        wdog_checkin(id);
        PRINTF("[W%d] working  t=%lu ms\r\n", id, (unsigned long)xTaskGetTickCount());
    }
}
