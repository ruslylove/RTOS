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
3. **Configure** the MCXN236 hardware watchdog (WWDT — Window Watchdog Timer) and observe a reset.
4. **Implement** a software watchdog pattern using task check-in flags.
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

### Hardware watchdog — WWDT

The MCXN236 uses a **Window Watchdog Timer (WWDT)** peripheral (`WWDT0`), clocked from the internal watchdog oscillator. It generates a system reset if the counter is not refreshed (kicked) within a configured window. This is the last line of defence against a hung system.

> **Note:** The MCXN236 does **not** have a WDOG32 module. The correct SDK driver is `fsl_wwdt.h`.

---

## Project Structure

```
lab05_watchdog/
├── Makefile
└── src/
    ├── main.c               ← task creation, reset-cause check
    ├── deadlock_tasks.c/.h  ← Part A & B
    ├── watchdog.c/.h        ← Part C & D: hardware + software watchdog
    └── FreeRTOSConfig.h
```

All console output uses `PRINTF()` from `fsl_debug_console.h` — no separate UART driver is required.

---

## Part A — Induce a Deadlock

### Step 1 — Implement the deadlock scenario

```c
/* main.c — set LAB_PART 1 */
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
        PRINTF("[A] taking MutexA...\r\n");
        xSemaphoreTake(xMutexA, portMAX_DELAY);
        PRINTF("[A] took MutexA. Taking MutexB...\r\n");
        vTaskDelay(pdMS_TO_TICKS(5));   /* window for B to acquire MutexB */
        xSemaphoreTake(xMutexB, portMAX_DELAY);
        PRINTF("[A] took both — doing work\r\n");
        xSemaphoreGive(xMutexB);
        xSemaphoreGive(xMutexA);
    }
}

void vTaskBeta(void *pv)   /* prio 2 */
{
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
        PRINTF("[B] taking MutexB...\r\n");
        xSemaphoreTake(xMutexB, portMAX_DELAY);
        PRINTF("[B] took MutexB. Taking MutexA...\r\n");
        vTaskDelay(pdMS_TO_TICKS(5));
        xSemaphoreTake(xMutexA, portMAX_DELAY);  /* ← opposite order to Task A */
        PRINTF("[B] took both — doing work\r\n");
        xSemaphoreGive(xMutexA);
        xSemaphoreGive(xMutexB);
    }
}
```

### Step 2 — Build, flash, and observe

```bash
make LAB_PART=1 && make flash
```

Watch the terminal. Within a few seconds, both `[A]` and `[B]` will stop printing "took both" — the system is deadlocked.

**Checkpoint A:** UART capture showing the last lines before deadlock, with both tasks blocked.

> **Question A1:** Draw the resource allocation graph at the moment of deadlock. Label each edge as an assignment or a request edge. Which Coffman condition does the circular edge represent?

> **Question A2:** The deadlock is **non-deterministic** — it does not always happen on the first iteration. What determines how quickly it manifests? Would the deadlock still be possible if both tasks ran at different priorities?

---

## Part B — Prevent with Resource Ordering

### Step 1 — Apply resource ordering

```c
/* deadlock_tasks.c — FIXED, LAB_PART 2 */
#define LOCK_ORDER_MUTEX_A  1
#define LOCK_ORDER_MUTEX_B  2
/* Rule: always acquire in ascending order */

void vTaskBeta(void *pv)   /* FIXED: acquire A before B */
{
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
        PRINTF("[B] taking MutexA (order fix)...\r\n");
        xSemaphoreTake(xMutexA, portMAX_DELAY);  /* A first */
        PRINTF("[B] taking MutexB...\r\n");
        xSemaphoreTake(xMutexB, portMAX_DELAY);  /* B second */
        PRINTF("[B] took both — doing work\r\n");
        xSemaphoreGive(xMutexB);
        xSemaphoreGive(xMutexA);
    }
}
```

Build and flash with:

```bash
make LAB_PART=2 && make flash
```

### Step 2 — Verify no deadlock

Run for at least 60 seconds. Both tasks should continue printing "took both" indefinitely.

**Checkpoint B:** UART capture of 10+ seconds of normal operation with no deadlock.

> **Question B1:** Resource ordering prevents deadlock by breaking which Coffman condition? Explain precisely why a total order on acquisition prevents a cycle in the resource-allocation graph.

> **Question B2:** Your system has 6 mutexes protecting 6 different subsystems. How do you choose the lock order? What property must the ordering have? Is it always possible?

---

## Part C — Hardware Watchdog (WWDT)

### Step 1 — Configure WWDT0

The MCXN236 WWDT module resets the chip if not refreshed within a timeout window. The watchdog oscillator frequency is queried at runtime via `CLOCK_GetWdtClkFreq(0)`.

```c
/* watchdog.c — LAB_PART 3 */
#include "fsl_wwdt.h"
#include "fsl_clock.h"

void WDOG_Init_1s(void)
{
    CLOCK_EnableClock(kCLOCK_Wwdt0);

    uint32_t wdtFreq = CLOCK_GetWdtClkFreq(0U);
    if (wdtFreq == 0U) wdtFreq = 1000000U;  /* fallback: 1 MHz */

    wwdt_config_t cfg;
    WWDT_GetDefaultConfig(&cfg);
    cfg.enableWwdt          = true;
    cfg.enableWatchdogReset = true;
    cfg.windowValue         = 0xFFFFFFU;  /* no window restriction */
    cfg.timeoutValue        = wdtFreq;    /* ~1 s timeout */
    cfg.clockFreq_Hz        = wdtFreq;
    WWDT_Init(WWDT0, &cfg);
}

void WDOG_Kick(void)
{
    WWDT_Refresh(WWDT0);
}
```

### Step 2 — Normal operation — kick the watchdog

```c
void vWatchdogTask(void *pv)  /* prio 4 — highest */
{
    WDOG_Init_1s();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
        WDOG_Kick();
        PRINTF("[WDG] kicked  t=%lu ms\r\n", (unsigned long)xTaskGetTickCount());
    }
}
```

Build and flash:

```bash
make LAB_PART=3 && make flash
```

Run for 10 seconds. The system should run normally with kick messages every 500 ms.

### Step 3 — Trigger the watchdog reset

Build with `SIMULATE_HANG=1`. When set, `vWatchdogTask` delays 2 seconds instead of kicking:

```c
#if SIMULATE_HANG
    PRINTF("[WDG] simulating hang — NOT kicking!\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000));   /* miss the deadline → watchdog fires */
#else
    WDOG_Kick();
#endif
```

```bash
make LAB_PART=3 CFLAGS_EXTRA="-DSIMULATE_HANG=1" && make flash
```

After the reset, the firmware checks `WWDT_GetStatusFlags(WWDT0)`:

```c
/* main.c — checked before scheduler starts */
if (WWDT_GetStatusFlags(WWDT0) & kWWDT_TimeoutFlag)
{
    PRINTF("[BOOT] *** last reset was caused by WWDT timeout ***\r\n");
    WWDT_ClearStatusFlags(WWDT0, kWWDT_TimeoutFlag);
}
else
{
    PRINTF("[BOOT] (no watchdog reset on this boot)\r\n");
}
```

**Checkpoint C:** UART capture showing `*** last reset was caused by WWDT timeout ***` on the reboot following the simulated hang.

> **Question C1:** The WWDT uses the internal watchdog oscillator (typically ~1 MHz on MCXN236). How does `timeoutValue = wdtFreq` give approximately 1 second? Why is a dedicated low-frequency oscillator used for the watchdog rather than the main 150 MHz PLL?

> **Question C2:** A **window watchdog** resets the chip if you kick too early OR too late. How would you set `cfg.windowValue` to require the kick to arrive only in the last 50 ms of the 1 s window? Sketch the timing diagram.

---

## Part D — Software Watchdog with Task Monitoring

### Step 1 — Multi-task monitoring pattern

```c
/* watchdog.c — LAB_PART 4 */
#define NUM_WORKERS  3
static volatile uint8_t worker_alive[NUM_WORKERS];

void wdog_checkin(uint8_t worker_id)
{
    if (worker_id < NUM_WORKERS)
        worker_alive[worker_id] = 1;
}

void vWatchdogMonitor(void *pv)  /* prio 4 */
{
    WDOG_Init_1s();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));

        bool all_ok = true;
        for (int i = 0; i < NUM_WORKERS; i++) {
            if (!worker_alive[i]) {
                PRINTF("[WDG] worker %d silent — NOT kicking!\r\n", i);
                all_ok = false;
            }
            worker_alive[i] = 0;
        }

        if (all_ok) {
            WDOG_Kick();
            PRINTF("[WDG] all workers OK — kicked  t=%lu ms\r\n",
                   (unsigned long)xTaskGetTickCount());
        }
    }
}

void vWorkerTask(void *pv)
{
    uint8_t id = (uint8_t)(uintptr_t)pv;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(200));

        /* Worker 1 hangs after 5 s to demonstrate detection */
        if (id == 1 && xTaskGetTickCount() > pdMS_TO_TICKS(5000)) {
            PRINTF("[W%d] hanging — no more check-ins!\r\n", id);
            for (;;) {}
        }

        wdog_checkin(id);
        PRINTF("[W%d] working  t=%lu ms\r\n", id, (unsigned long)xTaskGetTickCount());
    }
}
```

### Step 2 — Flash and observe

```bash
make LAB_PART=4 && make flash
```

Observe: after ~5 s, worker 1 hangs. The monitor reports it silent, stops kicking within 500 ms, and the WWDT fires ~1 s later, causing a reset.

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
| 4 | Part C UART capture — WWDT reset cause message on reboot |
| 5 | Part D UART capture — monitor detecting silent worker + reset |
| 6 | Written answers to Questions A1–A2, B1–B2, C1–C2, D1–D2 |

---

## Key Concepts Demonstrated

| Concept | Where you saw it |
|---------|-----------------|
| Deadlock induction — opposite lock order | Part A |
| Coffman condition 4 — circular wait | Part A diagram |
| Resource ordering — deadlock prevention | Part B |
| WWDT hardware watchdog configuration | Part C |
| Reset cause detection at boot | Part C |
| Software watchdog with per-task monitoring | Part D |

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| Coffman, Elphick & Shoshani (1971) | "System deadlocks" (*ACM Computing Surveys*) |
| Barry, *Mastering the FreeRTOS Kernel* | Ch. 6 (mutexes) |
| NXP MCXN236 Reference Manual | Chapter WWDT (Window Watchdog Timer) |
| NXP MCUXpresso SDK | `fsl_wwdt.h`, `WWDT_GetDefaultConfig`, `WWDT_Refresh` |
