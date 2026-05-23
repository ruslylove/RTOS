# Lab 05 — Deadlock Detection & Watchdog Timers

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** NXP FRDM-MCXN236 (Cortex-M33)  
**Estimated time:** 3 hours  
**Builds on:** Lab 04 (mutexes, priority inversion)

---

## Objectives

By completing this lab you will be able to:

1. **Induce** a deadlock between two tasks holding two mutexes in opposite order.
2. **Prevent** deadlock using the resource-ordering convention.
3. **Configure** the MCXN236 hardware watchdog (WDOG32) and observe a reset.
4. **Implement** a software watchdog pattern using task notifications.
5. **Isolate** a hung task — log its identity and trigger a controlled reset.

---

## Prerequisites

- [ ] Lab 04 complete — mutexes and SystemView working.
- [ ] ARM GNU Toolchain and `pyocd` working.

---

## Background

### Deadlock and the four Coffman conditions

A deadlock requires all four conditions simultaneously:

1. **Mutual exclusion** — resources are non-shareable.
2. **Hold and wait** — a task holds at least one resource while waiting for another.
3. **No preemption** — resources are released only voluntarily.
4. **Circular wait** — a cycle exists in the resource-allocation graph.

FreeRTOS mutexes satisfy conditions 1–3 by design. Your task is to prevent condition 4 using **resource ordering**: assign a global lock order and always acquire resources in ascending order.

### Hardware watchdog

The MCXN236 WDOG32 generates a system reset if not refreshed within a configured window. This is the last line of defence against a hung system.

---

## Project Structure

```
lab05_watchdog/
├── Makefile
├── linker_MCXN236.ld
├── src/
│   ├── main.c               ← task creation, WDOG init
│   ├── deadlock_tasks.c/.h  ← Part A & B
│   ├── watchdog.c/.h        ← Part C & D hardware + software watchdog
│   ├── FreeRTOSConfig.h
│   └── uart.h / uart.c
└── FreeRTOS-Kernel/
```

---

## Part A — Induce a Deadlock

### Step 1 — Implement the deadlock scenario

Create two mutexes and two tasks that acquire them in opposite order:

```c
/* main.c */
SemaphoreHandle_t xMutexA;
SemaphoreHandle_t xMutexB;

xMutexA = xSemaphoreCreateMutex();
xMutexB = xSemaphoreCreateMutex();
```

```c
/* deadlock_tasks.c */
void vTaskAlpha(void *pv)  /* prio 2 */
{
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
        uart_printf("[A] taking MutexA...\r\n");
        xSemaphoreTake(xMutexA, portMAX_DELAY);
        uart_printf("[A] took MutexA. Taking MutexB...\r\n");
        vTaskDelay(pdMS_TO_TICKS(5));   /* window for B to acquire MutexB */
        xSemaphoreTake(xMutexB, portMAX_DELAY);
        uart_printf("[A] took both — doing work\r\n");
        xSemaphoreGive(xMutexB);
        xSemaphoreGive(xMutexA);
    }
}

void vTaskBeta(void *pv)   /* prio 2 */
{
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
        uart_printf("[B] taking MutexB...\r\n");
        xSemaphoreTake(xMutexB, portMAX_DELAY);
        uart_printf("[B] took MutexB. Taking MutexA...\r\n");
        vTaskDelay(pdMS_TO_TICKS(5));
        xSemaphoreTake(xMutexA, portMAX_DELAY);  /* ← opposite order to Task A */
        uart_printf("[B] took both — doing work\r\n");
        xSemaphoreGive(xMutexA);
        xSemaphoreGive(xMutexB);
    }
}
```

### Step 2 — Build, flash, and observe

```bash
make setup && make && make flash
```

Watch the terminal. Within a few seconds, both `[A]` and `[B]` will stop printing "took both" — the system is deadlocked. The UART will show each task stuck waiting.

**Checkpoint A:** UART capture showing the last lines before deadlock, with both tasks blocked.

> **Question A1:** Draw the resource allocation graph at the moment of deadlock. Label each edge as an assignment or a request edge. Which Coffman condition does the circular edge represent?

> **Question A2:** The deadlock is **non-deterministic** — it does not always happen on the first iteration. What determines how quickly it manifests? Would the deadlock still be possible if both tasks ran at different priorities?

---

## Part B — Prevent with Resource Ordering

### Step 1 — Apply resource ordering

Assign a global lock order constant and enforce it in both tasks:

```c
/* deadlock_tasks.c — FIXED */
#define LOCK_ORDER_MUTEX_A  1
#define LOCK_ORDER_MUTEX_B  2
/* Rule: always acquire in ascending order */

void vTaskAlpha(void *pv)  /* unchanged — already A then B (correct order) */
{ /* ... same as before ... */ }

void vTaskBeta(void *pv)   /* FIXED: acquire A before B */
{
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
        uart_printf("[B] taking MutexA (order fix)...\r\n");
        xSemaphoreTake(xMutexA, portMAX_DELAY);  /* A first */
        uart_printf("[B] taking MutexB...\r\n");
        xSemaphoreTake(xMutexB, portMAX_DELAY);  /* B second */
        uart_printf("[B] took both — doing work\r\n");
        xSemaphoreGive(xMutexB);
        xSemaphoreGive(xMutexA);
    }
}
```

### Step 2 — Verify no deadlock

Run for at least 60 seconds. Both tasks should continue printing "took both" indefinitely.

**Checkpoint B:** UART capture of 10+ seconds of normal operation with no deadlock.

> **Question B1:** Resource ordering prevents deadlock by breaking which Coffman condition? Explain precisely why a total order on acquisition prevents a cycle in the resource-allocation graph.

> **Question B2:** Your system has 6 mutexes protecting 6 different subsystems. How do you choose the lock order? What property must the ordering have? Is it always possible?

---

## Part C — Hardware Watchdog

### Step 1 — Configure WDOG32

The MCXN236 WDOG32 module resets the chip if not refreshed within a timeout window.

```c
/* watchdog.c */
#include "fsl_wdog32.h"

void WDOG_Init_1s(void)
{
    wdog32_config_t cfg;
    WDOG32_GetDefaultConfig(&cfg);
    cfg.timeoutValue  = 0x100U;   /* ~1 second at LPO 128 Hz */
    cfg.enableWdog32  = true;
    cfg.clockSource   = kWDOG32_ClockSourceLpoClk;
    cfg.prescaler     = kWDOG32_ClockPrescalerDivide1;
    WDOG32_Init(WDOG0, &cfg);
}

void WDOG_Kick(void)
{
    WDOG32_Refresh(WDOG0);
}
```

### Step 2 — Normal operation — kick the watchdog

Create a high-priority watchdog task that kicks every 500 ms:

```c
void vWatchdogTask(void *pv)  /* prio 4 — highest */
{
    WDOG_Init_1s();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
        WDOG_Kick();
        uart_printf("[WDG] kicked\r\n");
    }
}
```

Run for 10 seconds. The system should run normally.

### Step 3 — Trigger the watchdog reset

Add a `#define SIMULATE_HANG 1` build flag. When set, `vWatchdogTask` sleeps for 2 seconds instead of kicking:

```c
#ifdef SIMULATE_HANG
    vTaskDelay(pdMS_TO_TICKS(2000));   /* miss the deadline → watchdog fires */
#else
    WDOG_Kick();
#endif
```

Build with `make HANG=1 && make HANG=1 flash`.

Observe the board reset. After reset, the firmware checks `WDOG32_GetStatusFlags(WDOG0)` and prints the reset cause:

```c
/* In main, before scheduler starts: */
if (WDOG32_GetStatusFlags(WDOG0) & kWDOG32_InterruptFlag) {
    uart_printf("[BOOT] Reset caused by watchdog!\r\n");
}
```

**Checkpoint C:** UART capture showing `[BOOT] Reset caused by watchdog!` on the reboot following the simulated hang.

> **Question C1:** The WDOG32 uses the LPO (Low-Power Oscillator) at 128 Hz. Calculate the exact timeout in milliseconds for `timeoutValue = 0x100`. Why is a low-power oscillator used rather than the main system clock?

> **Question C2:** A **window watchdog** adds a minimum refresh time — you cannot kick too early OR too late. How would you configure a window watchdog to catch a "too-fast" kick (indicating the kick loop has gone haywire)? Sketch the `cfg.windowValue` setting.

---

## Part D — Software Watchdog with Task Monitoring

### Step 1 — Multi-task monitoring pattern

The hardware watchdog is kicked by a dedicated monitor task that only kicks if **all** worker tasks have checked in during the last interval:

```c
/* watchdog.c — software monitor */
#define NUM_WORKERS  3
static volatile uint8_t  worker_alive[NUM_WORKERS];

/* Called by each worker task every cycle */
void wdog_checkin(uint8_t worker_id)
{
    worker_alive[worker_id] = 1;
}

void vWatchdogMonitor(void *pv)  /* prio 4 */
{
    WDOG_Init_1s();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Check all workers have checked in */
        bool all_ok = true;
        for (int i = 0; i < NUM_WORKERS; i++) {
            if (!worker_alive[i]) {
                uart_printf("[WDG] worker %d silent — NOT kicking!\r\n", i);
                all_ok = false;
            }
            worker_alive[i] = 0;   /* reset for next interval */
        }

        if (all_ok) {
            WDOG_Kick();
            uart_printf("[WDG] all workers OK — kicked\r\n");
        }
        /* If not all OK, watchdog expires → reset */
    }
}

/* Worker task (one of NUM_WORKERS) */
void vWorkerTask(void *pv)
{
    uint8_t id = (uint8_t)(uintptr_t)pv;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(200));
        wdog_checkin(id);
        uart_printf("[W%d] working\r\n", id);
    }
}
```

### Step 2 — Simulate a hanging worker

Add a flag to stop one worker checking in after 5 seconds:

```c
if (id == 1 && xTaskGetTickCount() > pdMS_TO_TICKS(5000)) {
    /* Hang — infinite loop, no checkin */
    for (;;) {}
}
```

Observe: after the hang, the monitor stops kicking within 500 ms, and the watchdog fires ~1 second later.

**Checkpoint D:** UART capture showing the monitor detecting the silent worker, then the board reset and reboot message.

> **Question D1:** In this pattern, the monitor task itself could hang. How would you guard against that? (Hint: think about who monitors the monitor.)

> **Question D2:** The worker uses a `volatile uint8_t` flag rather than a task notification. What are the pros and cons of each approach? Which is safer in an RTOS context and why?

---

## Deliverables

| # | Item |
|---|------|
| 1 | Part A UART capture — deadlock state visible |
| 2 | Part A resource-allocation graph diagram (hand-drawn or tool) |
| 3 | Part B UART capture — 60+ seconds of normal operation |
| 4 | Part C UART capture — watchdog reset cause message on reboot |
| 5 | Part D UART capture — monitor detecting silent worker + reset |
| 6 | Written answers to Questions A1–A2, B1–B2, C1–C2, D1–D2 |

---

## Key Concepts Demonstrated

| Concept | Where you saw it |
|---------|-----------------|
| Deadlock induction — opposite lock order | Part A |
| Coffman condition 4 — circular wait | Part A diagram |
| Resource ordering — deadlock prevention | Part B |
| WDOG32 hardware watchdog configuration | Part C |
| Reset cause detection at boot | Part C |
| Software watchdog with per-task monitoring | Part D |

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| Coffman, Elphick & Shoshani (1971) | "System deadlocks" (*ACM Computing Surveys*) |
| Barry, *Mastering the FreeRTOS Kernel* | Ch. 6 (mutexes) |
| NXP MCXN236 Reference Manual | Chapter WDOG32 |
| NXP MCUXpresso SDK | `fsl_wdog32.h`, `WDOG32_GetDefaultConfig` |
