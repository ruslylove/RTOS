/*
 * tickless_port.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 10 — Tickless Idle: Custom Port Hook
 *
 * When configUSE_TICKLESS_IDLE = 1, FreeRTOS calls vPortSuppressTicksAndSleep
 * from the idle task instead of executing a WFI with SysTick running.
 *
 * This module implements that hook using the LPTMR (Low-Power Timer) which
 * keeps running in low-power stop modes, allowing the MCU to enter VLPS or
 * STOP while still waking up on schedule.
 *
 * Student tasks:
 *   Part B Step 1 — Implement LPTMR_Init with the correct clock source.
 *   Part B Step 2 — Implement vPortSuppressTicksAndSleep:
 *                     a. Compute sleep duration in LPTMR ticks.
 *                     b. Program LPTMR compare register.
 *                     c. Enter WFI (or a deeper stop mode).
 *                     d. On wakeup, compute actual elapsed ticks and call
 *                        vTaskStepTick() to advance the kernel clock.
 *   Part B Step 3 — Measure current draw with a multimeter; compare to Part A.
 * ─────────────────────────────────────────────────────────────────────────────
 */
#ifndef TICKLESS_PORT_H
#define TICKLESS_PORT_H

#include <stdint.h>
#include "FreeRTOS.h"

/*
 * LPTMR_Init
 *
 * Initialise the Low-Power Timer for tickless wakeup.
 * Call once from main() before vTaskStartScheduler().
 *
 * Clock source: LPO at 1 kHz (1 ms resolution matches configTICK_RATE_HZ).
 * TODO (Part B Step 1): configure LPTMR_CSR, LPTMR_PSR, and enable clock gate.
 */
void LPTMR_Init_Tickless(void);

/*
 * vPortSuppressTicksAndSleep
 *
 * Called by the FreeRTOS idle task when configUSE_TICKLESS_IDLE = 1.
 * xExpectedIdleTime is the number of ticks until the next scheduled wakeup.
 *
 * This function MUST:
 *   1. Disable SysTick interrupt (prevent spurious tick during sleep).
 *   2. Program LPTMR to fire after xExpectedIdleTime ticks.
 *   3. Enter low-power mode (WFI or VLPS stop).
 *   4. On wakeup, determine whether the LPTMR fired or another interrupt did.
 *   5. Call vTaskStepTick(actual_sleep_ticks) to update the kernel tick count.
 *   6. Re-enable SysTick.
 *
 * TODO (Part B Step 2): implement the body of this function.
 */
void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime);

#endif /* TICKLESS_PORT_H */
