# Lab 03 — Aperiodic Servers & Producer-Consumer Pipeline

**Platform:** WeAct STM32H503CBT6 · **Estimated time:** 3–4 hours

## Objectives
1. Implement the **Deferred Interrupt Handler** pattern (Part A).
2. Implement a **Polling Server** to bound aperiodic response time (Part B).
3. Build a **Producer-Consumer pipeline** using queues and event groups (Part C).

---

## Part A — Deferred Interrupt Handler

An ISR should do minimal work. This pattern lets the ISR give a binary semaphore and return instantly; a dedicated high-priority task performs the actual work.

```
[Simulated ISR / timer] ─ xSemaphoreGive(xAperiodicSem) ──→ [vAperiodicHandlerTask]
                                                              (prio 5, blocks on sem)
```

Enable in `main.c`:
```c
#define ENABLE_PERIODIC_TASKS  1
#define ENABLE_PART_A          1
```

**Expected output** (Part A only):
```
[P1] tick=500
[P2] tick=1000
[APERIODIC] event #1 handled at t=1500 ms
[P1] tick=1000
[P2] tick=2000
[APERIODIC] event #2 handled at t=3000 ms
```

> **Q1:** Why must `vAperiodicHandlerTask` have higher priority than the periodic tasks?  
> What happens if its priority equals or is lower than the periodics?

---

## Part B — Polling Server

Replace Part A with a **Polling Server** (`vPollingServerTask`, period Ts = 100 ms).  
It dequeues and services all pending requests within each period, then sleeps until the next.

```c
#define ENABLE_PART_B  1
```

> **Q2:** Using the Polling Server schedulability condition  
> $U_{server} = C_s / T_s$ where $C_s$ = maximum service time per period,  
> what fraction of CPU does the server consume at Ts = 100 ms, Cs ≈ 2 ms?

---

## Part C — Pipeline with Event Groups

Three tasks form a data pipeline. Two sensors produce readings into a queue.  
The Logger drains the queue when signalled by either sensor's event bit.  
The Monitor waits for the LOG_DONE event to print statistics.

```c
#define ENABLE_PART_C  1
```

**Event bit usage:**

| Bit | Symbol | Set by | Cleared by |
|-----|--------|--------|------------|
| 0 | `EV_SENSOR1_READY` | `vSensor1Task` | `vLoggerTask` |
| 1 | `EV_SENSOR2_READY` | `vSensor2Task` | `vLoggerTask` |
| 2 | `EV_LOG_DONE` | `vLoggerTask` | `vMonitorTask` |

> **Q3:** If `EV_LOG_DONE` is never cleared, what happens to the Monitor task?

---

## Deliverables
| # | Item |
|---|------|
| 1 | Part A output: ≥ 5 aperiodic events with periodic tasks running |
| 2 | Part B output: server consuming requests each period |
| 3 | Part C output: sensor and logger interleaving correctly |
| 4 | Written answers to Q1–Q3 |
