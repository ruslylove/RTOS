/*
 * tickless_port.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 10 — Tickless Idle: Custom vPortSuppressTicksAndSleep
 *
 * This file provides the hardware-specific tickless idle implementation using
 * the MCXN236 LPTMR peripheral.  The LPTMR continues to run in low-power stop
 * modes and can wake the core after a programmable interval.
 *
 * Only compiled when configUSE_TICKLESS_IDLE = 1.
 *
 * Estimated power savings (typical, depends on peripheral activity):
 *   Run mode   ~15 mA @ 150 MHz
 *   VLPS mode  ~50 µA
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "tickless_port.h"

/* Only compile the tickless implementation when the kernel feature is on */
#if configUSE_TICKLESS_IDLE == 1

#include "fsl_debug_console.h"

/*
 * LPTMR clock frequency.
 * The LPO (Low-Power Oscillator) on the MCXN236 runs at approximately 1 kHz.
 * With configTICK_RATE_HZ = 1000 this gives 1 LPTMR tick per FreeRTOS tick.
 *
 * TODO (Part B Step 1): verify the actual LPO frequency with an oscilloscope
 * or by consulting the MCXN236 reference manual (RM Section "LPO Clock").
 * Adjust LPTMR_CLOCK_HZ if necessary.
 */
#define LPTMR_CLOCK_HZ  1000UL

/* Maximum ticks the LPTMR can sleep (16-bit compare register) */
#define LPTMR_MAX_TICKS  0xFFFFUL

/* ── LPTMR register base (MCXN236) ─────────────────────────────────────── */
/*
 * The NXP SDK defines LPTMR0 as the peripheral base pointer.
 * Include fsl_lptmr.h if using the driver; or access registers directly.
 *
 * TODO (Part B Step 1): include <fsl_lptmr.h> and add the driver to APP_SRCS
 * in the Makefile, or access LPTMR0->CSR / PSR / CMR directly.
 */

/* Flag set by the LPTMR ISR to distinguish LPTMR wakeup from other IRQs */
static volatile uint8_t s_lptmr_fired = 0U;

/* ── LPTMR_Init_Tickless ────────────────────────────────────────────────── */

void LPTMR_Init_Tickless(void)
{
    /*
     * TODO (Part B Step 1): configure and enable LPTMR.
     *
     * Using the NXP SDK driver (recommended):
     *
     *   #include "fsl_lptmr.h"
     *
     *   lptmr_config_t cfg;
     *   LPTMR_GetDefaultConfig(&cfg);
     *   cfg.prescalerClockSource = kLPTMR_PrescalerClock_1;  // LPO
     *   cfg.bypassPrescaler      = true;
     *   LPTMR_Init(LPTMR0, &cfg);
     *
     *   NVIC_SetPriority(LPTMR0_IRQn,
     *       configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1U);
     *   NVIC_EnableIRQ(LPTMR0_IRQn);
     *
     * Direct register access alternative (no SDK driver dependency):
     *
     *   // Enable LPTMR clock gate in PCC
     *   PCC->PCC_LPTMR0 |= PCC_PCC_LPTMR0_CGC_MASK;
     *   LPTMR0->CSR = 0;          // reset
     *   LPTMR0->PSR = LPTMR_PSR_PCS(1) | LPTMR_PSR_PBYP_MASK;  // LPO, no prescaler
     */
    PRINTF("[TICKLESS] LPTMR_Init_Tickless: TODO — implement in Part B Step 1\r\n");
}

/* ── LPTMR0_IRQHandler ──────────────────────────────────────────────────── */

void LPTMR0_IRQHandler(void)
{
    s_lptmr_fired = 1U;

    /*
     * TODO (Part B Step 1): clear the LPTMR interrupt flag.
     *
     * SDK: LPTMR_ClearStatusFlags(LPTMR0, kLPTMR_TimerCompareFlag);
     * Register: LPTMR0->CSR |= LPTMR_CSR_TCF_MASK;
     */
}

/* ── vPortSuppressTicksAndSleep ─────────────────────────────────────────── */

void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime)
{
    /*
     * xExpectedIdleTime: number of FreeRTOS ticks until the next task wakeup.
     * We sleep for at most xExpectedIdleTime ticks or LPTMR_MAX_TICKS,
     * whichever is smaller.
     */

    if (xExpectedIdleTime > (TickType_t)LPTMR_MAX_TICKS)
    {
        xExpectedIdleTime = (TickType_t)LPTMR_MAX_TICKS;
    }

    /* Enter a critical section to prevent tick interrupt racing the sleep */
    __disable_irq();

    /* If a task became ready since we entered idle, abort the sleep */
    if (eTaskConfirmSleepModeStatus() == eAbortSleep)
    {
        __enable_irq();
        return;
    }

    /*
     * TODO (Part B Step 2a): disable SysTick interrupt so a stray tick
     * does not fire during low-power sleep.
     *
     *   SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
     */

    /*
     * TODO (Part B Step 2b): program LPTMR compare register and start timer.
     *
     *   SDK:    LPTMR_SetTimerPeriod(LPTMR0, (uint32_t)xExpectedIdleTime);
     *           LPTMR_StartTimer(LPTMR0);
     *   Direct: LPTMR0->CMR = (uint32_t)xExpectedIdleTime - 1U;
     *           LPTMR0->CSR |= LPTMR_CSR_TEN_MASK | LPTMR_CSR_TIE_MASK;
     */

    s_lptmr_fired = 0U;

    /*
     * TODO (Part B Step 2c): enter low-power mode.
     *   Minimum: __WFI()  (wait-for-interrupt, core clock gated)
     *   Better:  configure SMC/PMC for VLPS (Very-Low-Power Stop) before WFI
     *            so the full power domain is gated.
     *
     *   __enable_irq() is called here to allow the LPTMR (or any other) IRQ
     *   to wake the core; interrupts are re-masked immediately after.
     */
    __enable_irq();
    __WFI();
    __disable_irq();

    /*
     * TODO (Part B Step 2d): stop the LPTMR and compute how many ticks
     * actually elapsed.
     *
     *   uint32_t elapsed;
     *   if (s_lptmr_fired) {
     *       elapsed = xExpectedIdleTime;
     *   } else {
     *       // Woken by another IRQ — read LPTMR counter for actual ticks
     *       elapsed = LPTMR_GetCurrentTimerCount(LPTMR0);
     *   }
     *   LPTMR_StopTimer(LPTMR0);
     *
     *   // Advance FreeRTOS tick count by the number of ticks we slept
     *   if (elapsed > 0U) {
     *       vTaskStepTick((TickType_t)elapsed);
     *   }
     */

    /*
     * TODO (Part B Step 2e): re-enable SysTick interrupt.
     *
     *   SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
     */

    __enable_irq();

    /* Placeholder diagnostic — remove once the full implementation is in */
    static uint32_t s_sleep_count = 0U;
    s_sleep_count++;
    if ((s_sleep_count % 10U) == 0U)
    {
        PRINTF("[TICKLESS] idle #%lu: expected=%lu ticks (TODO: implement sleep)\r\n",
               (unsigned long)s_sleep_count,
               (unsigned long)xExpectedIdleTime);
    }
}

#endif /* configUSE_TICKLESS_IDLE == 1 */
