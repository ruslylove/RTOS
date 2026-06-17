# Lab 05 — Watchdog & Task Health Monitor

**Platform:** WeAct STM32H503CBT6 · **Estimated time:** 2–3 hours

## Objectives
1. Configure and use the **IWDG** (Independent Watchdog) on STM32H503.
2. Implement a **software per-task watchdog** to detect individual task hangs.
3. Detect a simulated task hang and observe the IWDG hardware reset.

## IWDG Configuration

The STM32H503 IWDG uses the LSI oscillator (~32 kHz):

| Parameter | Value | Result |
|-----------|-------|--------|
| Prescaler | /256 | Counter clock ≈ 125 Hz |
| Reload | 249 | Timeout ≈ 2.0 s |

`vHealthyTask` calls `watchdog_kick()` (IWDG refresh) every 1 s — well within the 2 s window.

## Software Watchdog Table

| Slot | Task | Deadline |
|------|------|---------|
| 0 | `vHealthyTask` | 1500 ms |
| 1 | `vDeadlockTask` | 3000 ms |

`vWatchdogMonitor` calls `sw_watchdog_check()` every 500 ms and prints a warning when a task misses its deadline.

## Expected Timeline

```
t=0     : [BOOT] Normal power-on reset.
t=0–10s : [Healthy] alive every 1 s
           [Deadlock] kicking every 2 s
t=10 s  : [Deadlock] HUNG — stops kicking sw watchdog
t=10.5 s: [WDG] Task 'Deadlock' missed deadline!  (sw watchdog detects it)
t≈20 s  : Hardware IWDG reset fires because... wait, vHealthyTask is still alive.
```

> **IMPORTANT:** In this lab, the IWDG is only kicked by `vHealthyTask`.  
> So as long as `vHealthyTask` runs, the hardware won't reset.  
> The software watchdog catches the individual `vDeadlockTask` failure.

## Experiments

### Exp 1 — Hardware IWDG reset
Stop calling `watchdog_kick()` inside `vHealthyTask` (comment it out).  
Rebuild and flash. The MCU should reset after ~2 s.  
After reset, the banner should print `Last reset was IWDG watchdog reset!`.

> **Q1:** After the IWDG-induced reset, can the firmware detect it at boot?  
> Which HAL function and which flag indicate an IWDG reset?

### Exp 2 — Both tasks hung
Modify `vHealthyTask` to also stop after 5 s:
```c
if (elapsed < 5000) { watchdog_kick(); ... } else { for (;;); }
```
> **Q2:** After 2 s more (7 s total), the IWDG fires. Does it reset cleanly?  
> What does the system do if `vApplicationStackOverflowHook` fires instead?

### Exp 3 — Software watchdog architecture
Redesign so the Monitor task (not Healthy) kicks the IWDG — but only if  
**all** watched tasks are alive.
> **Q3:** What is the advantage of this design over each task kicking independently?

## Deliverables
| # | Item |
|---|------|
| 1 | Serial log: sw watchdog detecting DeadlockTask miss |
| 2 | Serial log showing IWDG reset boot message (Exp 1) |
| 3 | Written answers to Q1–Q3 |
| 4 | Modified Monitor-kicks-IWDG architecture (Exp 3) |
