# Lab 10 — Tickless Idle / Low-Power RTOS

**Platform:** WeAct STM32H503CBT6 · **Estimated time:** 2 hours

## Objectives
1. Enable and observe FreeRTOS **tickless idle** (WFI-based sleep).
2. Measure the difference in CPU active time with tickless idle ON vs OFF.
3. Understand how `vApplicationIdleHook` and `vApplicationTickHook` interact with tickless sleep.
4. Identify the trade-offs between sleep depth, wake latency, and peripheral constraints.

## Background

### The idle task problem
When all user tasks are blocked (waiting on delays, queues, semaphores), the FreeRTOS
idle task runs. Without tickless idle, the idle task spins in a loop and SysTick fires
every 1 ms — keeping the CPU at full power even when there is nothing to do.

### Tickless idle (`configUSE_TICKLESS_IDLE = 1`)
The FreeRTOS ARM Cortex-M port calculates the longest time any task will be blocked.
If that time exceeds `configEXPECTED_IDLE_TIME_BEFORE_SLEEP` ticks:

1. SysTick is **reprogrammed** to fire at the end of the sleep period (not every 1 ms).
2. The CPU executes `__WFI()` — power consumption drops to Sleep mode (~10 mA at 3.3 V vs ~90 mA active for H503 at 250 MHz).
3. When the next interrupt fires (reprogrammed SysTick or a peripheral IRQ), the port corrects the FreeRTOS tick count to account for the elapsed sleep time.
4. Execution resumes as if the ticks had fired normally.

### Sleep modes on STM32H503
| Mode | Clock | Wakeup | Current (typ.) |
|------|-------|--------|----------------|
| Run  | Full 250 MHz | — | ~90 mA |
| Sleep (WFI) | CPU halted, AHB/APB running | Any IRQ | ~10–25 mA |
| Stop 0 | Core clocks off, SRAM retained | EXTI, LPTIM | ~100 µA |

This lab uses **Sleep** mode (WFI) — the simplest; all peripherals including UART remain
operational with no extra configuration. Stop modes would require LPTIM as the wakeup
source and are not covered here.

## Expected Output

### With `configUSE_TICKLESS_IDLE = 1`:
```
=== RTOS Lab 10: Tickless Idle ===
configUSE_TICKLESS_IDLE = 1  (WFI sleep between tasks)
Tasks: LED=500ms  Sensor=2000ms  Stats=5000ms

[LED  ] blink #1  tick=500
[LED  ] blink #2  tick=1000
[Sensr] read #1 (2 s period)
[LED  ] blink #3  tick=1500
...

── Power Stats (last 5 s) ──
  Idle entries   : 14          ← few entries; CPU spent most time in WFI
  Tick ISR fires : 11 / 5000 expected
  Est. CPU active: ~0%         ← nearly all time was spent in WFI
```

### With `configUSE_TICKLESS_IDLE = 0` (for comparison):
```
── Power Stats (last 5 s) ──
  Idle entries   : 4892813     ← idle task spinning continuously
  Tick ISR fires : 5000 / 5000 expected
  Est. CPU active: ~100%
```

## Experiments

### Exp 1 — Enable vs disable tickless idle
1. Build and run with `configUSE_TICKLESS_IDLE = 1` (default). Record stats.
2. Change to `configUSE_TICKLESS_IDLE = 0`, rebuild, and flash. Record stats.

> **Q1:** What is the ratio of idle entries per second between the two modes?  
> What does this tell you about how frequently the CPU was running the idle task?

### Exp 2 — Effect of task periods on sleep depth
Shorten `SENSOR_PERIOD_MS` to 100 ms (10 Hz sensor poll).

> **Q2:** How do the tick-ISR count and idle-entry count change?  
> Why does a shorter task period reduce the effectiveness of tickless idle?

### Exp 3 — Minimum idle time threshold
Change `configEXPECTED_IDLE_TIME_BEFORE_SLEEP` from 2 to 20 (20 ms threshold).

> **Q3:** What effect does a larger minimum idle threshold have on:  
> (a) the idle entry count, (b) task wakeup latency, (c) power consumption?

### Exp 4 — LED behaviour under tickless idle
With tickless idle enabled, the LED should still blink at exactly 500 ms.

> **Q4:** Explain how the tick count remains accurate when SysTick is suppressed  
> during sleep. What mechanism does the FreeRTOS port use to correct it?

## Deliverables
| # | Item |
|---|------|
| 1 | Serial log — stats output for `configUSE_TICKLESS_IDLE = 1` and `= 0` (Exp 1) |
| 2 | Stats output with `SENSOR_PERIOD_MS = 100` (Exp 2) |
| 3 | Written answers to Q1–Q4 |

## Implementation Notes

- `vApplicationIdleHook()` is called once each time the idle task runs, **before** the
  port enters WFI. With tickless idle it fires far less frequently than without.
- `vApplicationTickHook()` is called from `SysTick_Handler`. With tickless idle,
  SysTick is suppressed during sleep, so the hook fires far fewer than 1000×/s.
  `HAL_IncTick()` must still be called here (not in a task) to keep `HAL_Delay()` and
  `HAL_GetTick()` working correctly.
- **Do not** call `HAL_PWR_EnterSLEEPMode` inside `vApplicationIdleHook`. The FreeRTOS
  ARM Cortex-M port calls `__WFI()` internally via `vPortSuppressTicksAndSleep()` when
  tickless idle is active. Double-calling WFI would interfere with tick accounting.
- UART remains active during WFI sleep — any UART activity wakes the CPU immediately.
  If deep Stop mode is needed, the UART clock must be kept running via LPUART1.
