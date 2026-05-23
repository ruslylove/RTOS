#ifndef WATCHDOG_H
#define WATCHDOG_H

/*
 * watchdog.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 05, Parts C & D
 *
 * Part C — Hardware WDOG32:
 *   WDOG_Init_1s() : configure WDOG32 with ~1 s timeout (LPO 128 Hz)
 *   WDOG_Kick()    : refresh (strobe) the watchdog
 *   vWatchdogTask  : kicks every 500 ms; can simulate a hang via SIMULATE_HANG
 *
 * Part D — Software watchdog monitor:
 *   wdog_checkin()        : called by each worker task each cycle
 *   vWatchdogMonitor()    : verifies all workers checked in, then kicks WDOG32
 *   vWorkerTask()         : worker that calls wdog_checkin (id passed via pv)
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/* Number of worker tasks monitored in Part D */
#define NUM_WORKERS  3

/* ── Part C — Hardware watchdog ──────────────────────────────────────────── */
void WDOG_Init_1s(void);
void WDOG_Kick(void);
void vWatchdogTask(void *pvParameters);     /* Part C simple task */

/* ── Part D — Software watchdog monitor ──────────────────────────────────── */
void wdog_checkin(uint8_t worker_id);       /* called by each worker */
void vWatchdogMonitor(void *pvParameters);  /* monitor task — priority 4 */
void vWorkerTask(void *pvParameters);       /* generic worker — id via pv */

#endif /* WATCHDOG_H */
