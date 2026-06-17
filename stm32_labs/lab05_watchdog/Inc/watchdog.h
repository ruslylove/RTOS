#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdint.h>

/*
 * IWDG configuration for STM32H503:
 *   LSI ≈ 32 kHz, prescaler /256 → ~125 Hz counter.
 *   Reload = 250 → timeout ≈ 2000 ms.
 *   Tasks must call watchdog_kick() within every 2 s window.
 */
#define WDG_TIMEOUT_MS  2000U

void watchdog_init(void);
void watchdog_kick(void);  /* refresh the IWDG counter */

/* ── Software watchdog for individual tasks ─────────────────────────────── */
#define MAX_WATCHED_TASKS  4

typedef struct {
    const char *name;
    uint32_t    last_kick_ms;
    uint32_t    deadline_ms;  /* max time between kicks */
    uint8_t     alive;
} TaskWatchdog_t;

void sw_watchdog_register(uint8_t slot, const char *name, uint32_t deadline_ms);
void sw_watchdog_kick(uint8_t slot);
void sw_watchdog_check(void);  /* call from a monitor task */

#endif
