#ifndef DWT_H
#define DWT_H

#include <stdint.h>
#include "core_cm33.h"   /* from CMSIS-Core */

/*
 * DWT Cycle Counter helpers for Cortex-M33.
 *
 * The DWT (Data Watchpoint and Trace) unit provides a 32-bit free-running
 * cycle counter at DWT->CYCCNT.  At 250 MHz, it wraps every ~17 s.
 *
 * Usage:
 *   dwt_init();
 *   dwt_reset();
 *   my_function();
 *   uint32_t cycles = dwt_cycles();
 *   float us = (float)cycles / 250.0f;
 */

static inline void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* enable trace */
    DWT->CYCCNT  = 0;
    DWT->CTRL   |= DWT_CTRL_CYCCNTENA_Msk;           /* enable cycle counter */
}

static inline void dwt_reset(void)
{
    DWT->CYCCNT = 0;
}

static inline uint32_t dwt_cycles(void)
{
    return DWT->CYCCNT;
}

/* Convert cycles to microseconds at 250 MHz */
static inline float dwt_us(uint32_t cycles)
{
    return (float)cycles / 250.0f;
}

/* Convert cycles to nanoseconds at 250 MHz */
static inline uint32_t dwt_ns(uint32_t cycles)
{
    return cycles * 4U;   /* 1 cycle = 4 ns at 250 MHz */
}

#endif /* DWT_H */
