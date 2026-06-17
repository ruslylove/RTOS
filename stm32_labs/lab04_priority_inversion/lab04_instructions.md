# Lab 04 — Priority Inversion

**Platform:** WeAct STM32H503CBT6 · **Estimated time:** 2–3 hours

## Objectives
1. Observe **unbounded priority inversion** using a binary semaphore.
2. Resolve it using a FreeRTOS **priority-inheriting mutex**.
3. Measure how long HIGH is blocked in each case.

## Scenario

Three tasks contend for `xSharedResource`:

| Task | Priority | Behaviour |
|------|----------|-----------|
| `vLowPrioTask` | 1 | Acquires resource, does 200 ms CPU work |
| `vMedPrioTask` | 3 | CPU-bound (no resource); preempts LOW |
| `vHighPrioTask` | 5 | Requests resource that LOW holds |

**Timeline (binary semaphore — inversion):**
```
t=5000: LOW acquires resource
t=5050: HIGH wakes, blocks on resource (LOW holds it)
t=5100: MED wakes, preempts LOW (MED has no resource need!)
         HIGH is stuck until MED finishes (150 ms) + LOW finishes (200 ms)
Total blocking for HIGH ≈ 350 ms instead of ≤ 200 ms
```

**With mutex (priority inheritance):**
```
t=5050: HIGH blocks on resource → FreeRTOS raises LOW's priority to 5
        MED cannot preempt LOW now (MED prio 3 < raised LOW prio 5)
        LOW finishes in ≤ 200 ms → HIGH unblocks quickly
```

## Step 1 — Observe Inversion

In `inversion_tasks.h`, leave `USE_MUTEX` **undefined** (binary semaphore).  
Build, flash, and record how long HIGH reports being blocked.

> **Q1:** How long was HIGH blocked? Why did MED extend the blocking time?

## Step 2 — Fix with Mutex

Uncomment `#define USE_MUTEX` in `inversion_tasks.h`. Rebuild and flash.

> **Q2:** How long is HIGH blocked now? What did FreeRTOS do to LOW's priority  
> while HIGH was waiting?

## Step 3 — Deadlock Scenario

Create a **deadlock** by having HIGH and LOW each hold one resource and  
request the other:
1. Add a second mutex `xResourceB`.
2. LOW: acquire A then B. HIGH: acquire B then A.
3. Stagger start times so they interleave.

> **Q3:** What happens to the system? How would you detect this at run-time  
> using `xSemaphoreTake` with a finite timeout?

## Deliverables
| # | Item |
|---|------|
| 1 | Serial log showing HIGH blocked time with binary semaphore |
| 2 | Serial log showing reduced blocking with mutex |
| 3 | Written answers to Q1–Q3 |
| 4 | Modified `main.c` with deadlock demo and timeout-based detection |
