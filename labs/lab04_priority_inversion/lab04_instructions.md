# Lab 04 — Priority Inversion: See It, Fix It

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** NXP FRDM-MCXN236 (Cortex-M33)  
**Estimated time:** 2–3 hours  
**Builds on:** Lab 03 (queues, semaphores, SystemView)

---

## Objectives

By completing this lab you will be able to:

1. **Reproduce** the classic three-task priority inversion scenario on real hardware.
2. **Observe** the inversion window in a SEGGER SystemView trace and measure its duration.
3. **Quantify** the difference between theoretical worst-case response time with and without PIP.
4. **Fix** the inversion by replacing a binary semaphore with a FreeRTOS mutex.
5. **Verify** the fix — τH's response time must not exceed τL's critical-section length.

---

## Prerequisites

- [ ] Lab 03 complete — SystemView setup working.
- [ ] SEGGER SystemView running and connected to the board.
- [ ] ARM GNU Toolchain and `pyocd` working.

---

## Background

Priority inversion occurs when a high-priority task (τH) blocks on a resource held by a low-priority task (τL), and a medium-priority task (τM) preempts τL — causing τH to wait for τM, even though τM shares no resource with τH.

**Without PIP:**  
Response time of τH ≥ duration of τM's execution + τL's remaining critical section.  
This can be arbitrarily long — τM may run for as long as it likes.

**With PIP (FreeRTOS mutex):**  
When τH blocks, τL inherits τH's priority. τM cannot preempt τL. τH's blocking is bounded to at most one τL critical section.

---

## Project Structure

```
lab04_priority_inversion/
├── Makefile
├── linker_MCXN236.ld
├── src/
│   ├── main.c               ← task creation, kernel object init
│   ├── inversion_tasks.c/.h ← τH, τM, τL implementations
│   ├── FreeRTOSConfig.h
│   └── uart.h / uart.c
└── FreeRTOS-Kernel/
```

### Task parameters

| Task | Priority | Period | Critical section |
|------|----------|--------|-----------------|
| τH (high) | 3 | 200 ms | Takes shared resource for 5 ms |
| τM (medium) | 2 | 150 ms | Runs 30 ms of computation (no shared resource) |
| τL (low) | 1 | 300 ms | Holds shared resource for 40 ms |

---

## Part A — Reproduce the Inversion (Binary Semaphore)

### Step 1 — Implement the three tasks

In `inversion_tasks.c`, use a **binary semaphore** (`xSemaphoreCreateBinary`, pre-given to `pdTRUE`) as the shared resource guard:

```c
void vTaskH(void *pv)   /* prio 3 */
{
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(200));
        TickType_t t_arrive = xTaskGetTickCount();
        uart_printf("[H] arrived at %lu ms\r\n", t_arrive);

        xSemaphoreTake(xSharedResource, portMAX_DELAY);  /* may block here */
        TickType_t t_acquire = xTaskGetTickCount();
        uart_printf("[H] acquired resource at %lu ms  (waited %lu ms)\r\n",
                    t_acquire, t_acquire - t_arrive);

        vTaskDelay(pdMS_TO_TICKS(5));   /* critical section */
        xSemaphoreGive(xSharedResource);
        uart_printf("[H] released resource\r\n");
    }
}

void vTaskM(void *pv)   /* prio 2 */
{
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(150));
        uart_printf("[M] running (no resource held)\r\n");
        /* Simulate 30 ms computation — busy-wait to prevent sleep */
        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(30)) {}
        uart_printf("[M] done\r\n");
    }
}

void vTaskL(void *pv)   /* prio 1 */
{
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(300));
        xSemaphoreTake(xSharedResource, portMAX_DELAY);
        uart_printf("[L] acquired resource — holding for 40 ms\r\n");
        /* Simulate 40 ms critical section */
        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(40)) {}
        xSemaphoreGive(xSharedResource);
        uart_printf("[L] released resource\r\n");
    }
}
```

> **Timing note:** Tasks are offset at startup so that τL acquires the resource first, τH arrives while τL holds it, and τM preempts τL shortly after. This is handled in `main.c` using initial `vTaskDelay` offsets.

### Step 2 — Build and flash

```bash
make setup && make && make flash
```

### Step 3 — Capture UART output

Watch for lines where τH's `(waited X ms)` value is much larger than the 40 ms τL critical section alone. The extra time is the inversion window caused by τM.

**Example of inversion output:**
```
[L] acquired resource — holding for 40 ms
[H] arrived at 100 ms
[M] running (no resource held)
[M] done
[L] released resource
[H] acquired resource at 170 ms  (waited 70 ms)   ← 70 ms > 40 ms CS
```

### Step 4 — Capture SystemView trace

Open SystemView, start recording, let the system run for at least 3 seconds, then stop.

In the trace, find an inversion event:
- Identify the moment τH blocks on the semaphore.
- Identify the moment τM preempts τL.
- Measure the **inversion window** — the time τH waits while τM is running.

**Checkpoint A:** Annotated SystemView screenshot with the inversion window marked and measured.

---

## Part B — Theoretical Analysis

Before fixing the system, compute the worst-case response time analytically.

**Without PIP**, τH's response time can be bounded as:

$$R_H \leq C_H + B_H + \sum_{j \in \text{hp}(H)} \left\lceil \frac{R_H}{T_j} \right\rceil C_j$$

where $B_H$ is the **blocking term** — the maximum duration τH can be blocked by a lower-priority task. Without PIP, $B_H$ is unbounded (τM can always preempt τL).

**With PIP**, $B_H$ is bounded to the longest critical section of any lower-priority task that shares the resource with τH:

$$B_H = \max(\text{CS of lower-priority tasks sharing the resource}) = 40 \text{ ms}$$

> **Question B1:** Using the task parameters from the table, apply the RTA recurrence to compute τH's worst-case response time **with PIP**. Show your iteration steps.

> **Question B2:** From your UART output in Part A, what is the maximum `waited X ms` value you observed across multiple periods? Does it match the theoretical without-PIP bound?

> **Question B3:** Compute total utilisation $U$ for the three-task set. Is it schedulable under RMS? Does priority inversion change the schedulability answer? (Hint: yes and no — explain.)

---

## Part C — Fix with a Mutex

### Step 1 — Replace binary semaphore with mutex

In `main.c`, change one line:

```c
/* BEFORE (binary semaphore — no PIP): */
xSharedResource = xSemaphoreCreateBinary();
xSemaphoreGive(xSharedResource);   /* initialise as available */

/* AFTER (mutex — PIP enabled): */
xSharedResource = xSemaphoreCreateMutex();
```

Also ensure `FreeRTOSConfig.h` has:
```c
#define configUSE_MUTEXES  1
```

No other code changes needed — `xSemaphoreTake` and `xSemaphoreGive` have the same API for both mutexes and binary semaphores.

### Step 2 — Rebuild and flash

```bash
make && make flash
```

### Step 3 — Observe the fix

In the terminal, τH's `waited` time should now be ≤ 40 ms (the length of τL's critical section). τM cannot preempt τL while τL holds the mutex.

**Example of fixed output:**
```
[L] acquired resource — holding for 40 ms
[H] arrived at 100 ms
[H] acquired resource at 140 ms  (waited 40 ms)   ← bounded to τL's CS
[M] running (no resource held)
[M] done
```

### Step 4 — Capture the fixed SystemView trace

Capture a new trace. In the fixed trace:
- τL's priority **spikes to τH's priority level** when τH blocks (visible in SystemView as a priority change event).
- τM cannot preempt τL during this period.
- τH's wait time equals τL's remaining critical section.

**Checkpoint C:** Annotated SystemView screenshot showing the priority inheritance event (τL running at τH's priority).

---

## Part D — Experiments

### Experiment 1 — Deep nesting

Add a **second** shared resource `xResource2` (also a mutex). Modify τL to hold both resources:

```c
xSemaphoreTake(xSharedResource, portMAX_DELAY);
vTaskDelay(pdMS_TO_TICKS(10));
xSemaphoreTake(xResource2, portMAX_DELAY);
/* ... critical section ... */
xSemaphoreGive(xResource2);
xSemaphoreGive(xSharedResource);
```

And τM to also compete for `xResource2`.

> **Question D1:** FreeRTOS PIP documentation states that the current implementation propagates inheritance only **one level deep** — it does not fully propagate chained inheritance. Describe a scenario with this two-resource design where τH still experiences unexpected delay even with mutexes.

### Experiment 2 — ISR give

Replace the `xSemaphoreGive` inside `vTaskL` with a give from a software timer callback (simulating an ISR-side release).

> **Question D2:** `xSemaphoreGiveFromISR` does **not** trigger priority inheritance restoration. After τL "gives" from the ISR callback, does τL's priority return to its original level immediately? What does this tell you about using mutexes for resources released from ISR context?

---

## Deliverables

| # | Item |
|---|------|
| 1 | UART capture from Part A — at least one inversion event with `waited` value annotated |
| 2 | Part A annotated SystemView screenshot — inversion window measured |
| 3 | Part B written analysis — RTA calculation with and without PIP |
| 4 | Part C UART capture — `waited` bounded to ≤ 40 ms |
| 5 | Part C annotated SystemView screenshot — priority inheritance event visible |
| 6 | Written answers to Questions B1–B3, D1–D2 |

---

## Key Concepts Demonstrated

| Concept | Where you saw it |
|---------|-----------------|
| Three-task priority inversion scenario | Part A |
| Inversion window measurement | Part A SystemView trace |
| RTA blocking term $B_H$ | Part B analysis |
| Mutex vs binary semaphore for PIP | Part C code change |
| Priority inheritance in SystemView | Part C trace |
| Limits of FreeRTOS PIP (one-level) | Experiment D1 |

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| Sha, Rajkumar & Lehoczky (1990) | "Priority Inheritance Protocols" (*IEEE Trans. Computers*) |
| Buttazzo, *Hard Real-Time Computing Systems* | Ch. 7.1–7.3 |
| FreeRTOS docs | `xSemaphoreCreateMutex`, `configUSE_MUTEXES` |
| SEGGER SystemView manual | Priority display, event annotations |
