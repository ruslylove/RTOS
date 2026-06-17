# Lab 06 — WCET Measurement with DWT Cycle Counter

**Platform:** WeAct STM32H503CBT6 · **Estimated time:** 2–3 hours

## Objectives
1. Enable and use the **DWT CYCCNT** register on Cortex-M33.
2. Measure Worst-Case Execution Time (WCET) of several workloads.
3. Measure **FreeRTOS semaphore give→wake latency** in CPU cycles.
4. Compare measurements with and without the instruction cache enabled.

## DWT Cycle Counter

The DWT (Data Watchpoint and Trace) unit provides a 32-bit cycle counter at `DWT->CYCCNT`.

| Step | Code |
|------|------|
| Enable | `CoreDebug->DEMCR \|= CoreDebug_DEMCR_TRCENA_Msk; DWT->CTRL \|= DWT_CTRL_CYCCNTENA_Msk;` |
| Reset | `DWT->CYCCNT = 0;` |
| Read | `uint32_t c = DWT->CYCCNT;` |

At 250 MHz: **1 cycle = 4 ns**.  CYCCNT wraps every ~17 s.

## Expected Results (approximate, with I-cache enabled)

| Workload | Cycles | Time |
|----------|--------|------|
| `sqrtf` (FPU) | ~15 | ~60 ns |
| `memcpy` 256 B | ~40 | ~160 ns |
| FIR 16-tap/64 samples | ~2500 | ~10 µs |
| Bubble-sort 128 items | ~10000–15000 | ~40–60 µs |
| Dot-product 32 floats | ~100 | ~400 ns |
| Semaphore give→wake | ~200–400 | ~1–2 µs |

## Experiments

### Exp 1 — Cache effect
Disable the instruction cache by removing `HAL_ICACHE_Enable()` from `main.c`.
Rebuild (use Release build for a fairer comparison).

> **Q1:** By what factor does the FIR filter slow down without the I-cache?  
> Which workloads benefit most from the cache and why?

### Exp 2 — Debug vs Release
Build the same code in Debug (`-O0`) and Release (`-Os`) configurations.
```bash
cmake --preset Debug  && cmake --build build/Debug
cmake --preset Release && cmake --build build/Release
```
> **Q2:** For the bubble-sort, what is the speedup ratio Release/Debug?  
> Is it safe to use Release WCET measurements in a real-time system analysis?

### Exp 3 — Schedulability check
Assume tasks with the following parameters measured in Experiment 1:

| Task | C (WCET) | T (period) |
|------|----------|-----------|
| FIR filter | measured | 10 ms |
| Memcpy | measured | 5 ms |

Using Rate Monotonic Analysis, check if this task set is schedulable:
$U = \sum C_i / T_i \leq n(2^{1/n} - 1)$

> **Q3:** Is the task set schedulable? What is the utilization bound for n=2?

### Exp 4 — Task-switch latency
The `vLatencyTask` measures the cycle gap between `xSemaphoreGive` and the  
moment the receiving task reads `DWT->CYCCNT` after waking.

> **Q4:** What is the semaphore give→wake latency in µs?  
> How does it compare to the FreeRTOS tick period (1 ms)?

## Deliverables
| # | Item |
|---|------|
| 1 | Table of WCET measurements (Debug and Release, with/without cache) |
| 2 | Semaphore latency measurement |
| 3 | Schedulability analysis (Exp 3) |
| 4 | Written answers to Q1–Q4 |
