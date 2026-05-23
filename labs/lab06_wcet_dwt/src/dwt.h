#ifndef DWT_H
#define DWT_H

/*
 * dwt.h — ARM DWT Cycle Counter helpers
 * ─────────────────────────────────────────────────────────────────────────────
 * The Data Watchpoint and Trace (DWT) unit on the ARM Cortex-M33 provides a
 * 32-bit free-running cycle counter. This header provides complete inline
 * implementations — no .c file required.
 *
 * Usage:
 *   DWT_Enable();              // call once at startup (before scheduler)
 *
 *   DWT_START();               // saves current cycle count into _dwt_start
 *   do_some_work();
 *   DWT_STOP();                // computes elapsed into _dwt_elapsed
 *   uart_printf("%lu cycles\r\n", _dwt_elapsed);
 *
 * At 150 MHz: 1 cycle = 6.67 ns.
 * Counter rolls over after 2^32 cycles ~ 28.6 seconds.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>
#include "core_cm33.h"   /* CoreDebug, DWT from CMSIS */

/* ── Enable the DWT cycle counter ─────────────────────────────────────────── */
static inline void DWT_Enable(void)
{
    /* Step 1: enable the DWT and ITM blocks via CoreDebug */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* Step 2: reset the counter */
    DWT->CYCCNT = 0U;

    /* Step 3: start counting */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* Ensure the writes are visible before the first read */
    __DSB();
    __ISB();
}

/* ── Read the raw cycle counter ───────────────────────────────────────────── */
static inline uint32_t DWT_GetCycles(void)
{
    return DWT->CYCCNT;
}

/* ── Convenience macros ───────────────────────────────────────────────────── */

/*
 * DWT_START() — declare and capture start time.
 * Must be used before DWT_STOP() in the same scope.
 */
#define DWT_START() \
    uint32_t _dwt_start = DWT_GetCycles()

/*
 * DWT_STOP() — declare and compute elapsed cycles.
 * After this macro, use _dwt_elapsed (uint32_t).
 */
#define DWT_STOP() \
    uint32_t _dwt_elapsed = DWT_GetCycles() - _dwt_start

/*
 * DWT_CYCLES_TO_US(cycles) — convert cycle count to microseconds at 150 MHz.
 * Returns a float; cast to uint32_t for integer arithmetic.
 */
#define DWT_CYCLES_TO_US(cycles)  ((float)(cycles) / 150.0f)

/*
 * DWT_CYCLES_TO_NS(cycles) — convert cycle count to nanoseconds at 150 MHz.
 */
#define DWT_CYCLES_TO_NS(cycles)  ((float)(cycles) * (1000.0f / 150.0f))

#endif /* DWT_H */
