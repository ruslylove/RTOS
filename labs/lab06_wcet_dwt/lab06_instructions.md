# Lab 06 — WCET Measurement with DWT & Interrupt Latency

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** NXP FRDM-MCXN236 (Cortex-M33)  
**Estimated time:** 3–4 hours  
**Builds on:** Lab 03 (producer-consumer pipeline)

---

## Objectives

By completing this lab you will be able to:

1. **Enable and read** the DWT cycle counter on the ARM Cortex-M33 at 150 MHz.
2. **Measure** best-case, average, and worst-case execution time for a data-processing function over 1 000 runs.
3. **Apply** a WCET safety margin and use the result as $C_i$ in schedulability analysis.
4. **Measure** interrupt latency from IRQ assertion to first ISR instruction using an oscilloscope or logic analyser.
5. **Compare** measured interrupt latency with and without a FreeRTOS critical section active.

---

## Prerequisites

- [ ] Lab 03 complete.
- [ ] ARM GNU Toolchain and `pyocd` working.
- [ ] Oscilloscope or logic analyser with ≥ 4 channels (for Part B). A probe on GPIO pins is sufficient.
- [ ] FRDM-MCXN236 board — no other hardware required for Part A.

---

## Background

### Why measure WCET?

The Response-Time Analysis from Week 3 requires a bound $C_i$ on task execution time. In practice, $C_i$ is obtained by:

1. **Static analysis** — examine all paths, compute worst-case bound. Requires special tools (aiT, Bound-T).
2. **Measurement-based** — run the function many times with controlled inputs, take the maximum plus a safety margin.

This lab uses measurement-based WCET — practical for most systems below SIL-3/ASIL-D.

### DWT cycle counter

The ARM Data Watchpoint and Trace (DWT) unit has a 32-bit free-running cycle counter. Steps to enable:

```c
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* enable DWT */
DWT->CYCCNT = 0;
DWT->CTRL   |= DWT_CTRL_CYCCNTENA_Msk;            /* start counting */
```

Read: `uint32_t cycles = DWT->CYCCNT;`

At 150 MHz, 1 cycle = 6.67 ns. The counter rolls over after 2³² cycles ≈ 28.6 seconds.

---

## Project Structure

```
lab06_wcet_dwt/
├── Makefile
├── linker_MCXN236.ld
├── src/
│   ├── main.c              ← DWT init, task creation
│   ├── dwt.h               ← DWT enable/read macros
│   ├── workload.c/.h       ← the functions to be measured
│   ├── latency_test.c/.h   ← Part B: GPIO ISR latency test
│   ├── FreeRTOSConfig.h
│   └── uart.h / uart.c
└── FreeRTOS-Kernel/
```

---

## Part A — WCET Measurement

### Step 1 — Enable DWT

In `dwt.h`:

```c
#ifndef DWT_H
#define DWT_H
#include "core_cm33.h"

static inline void DWT_Enable(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB();
    __ISB();
}

static inline uint32_t DWT_GetCycles(void)
{
    return DWT->CYCCNT;
}

#define DWT_START()  uint32_t _dwt_start = DWT_GetCycles()
#define DWT_STOP()   uint32_t _dwt_elapsed = DWT_GetCycles() - _dwt_start
#endif
```

### Step 2 — Implement the workload functions

Three functions of increasing complexity in `workload.c`:

```c
/* Function 1: bubble sort — data-dependent WCET */
void bubble_sort(int16_t *arr, uint16_t n)
{
    for (uint16_t i = 0; i < n - 1; i++)
        for (uint16_t j = 0; j < n - i - 1; j++)
            if (arr[j] > arr[j+1]) {
                int16_t tmp = arr[j];
                arr[j]   = arr[j+1];
                arr[j+1] = tmp;
            }
}

/* Function 2: 16-tap FIR filter — fixed WCET */
int32_t fir_filter(const int16_t *x, const int16_t *h, uint16_t n)
{
    int32_t acc = 0;
    for (uint16_t i = 0; i < n; i++)
        acc += (int32_t)x[i] * h[i];
    return acc >> 15;
}

/* Function 3: 32-bit CRC computation — data-dependent */
uint32_t crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    while (len--) {
        crc ^= *data++;
        for (int k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1));
    }
    return ~crc;
}
```

### Step 3 — Measurement task

```c
#define NUM_RUNS     1000
#define ARRAY_SIZE   100

void vWcetMeasureTask(void *pv)
{
    static int16_t arr[ARRAY_SIZE];
    static int16_t sorted[ARRAY_SIZE];
    static int16_t reverse[ARRAY_SIZE];
    static int16_t random_arr[ARRAY_SIZE];
    static int16_t fir_h[16] = {1,2,3,4,5,6,7,8,8,7,6,5,4,3,2,1};

    /* Prepare test inputs */
    for (int i = 0; i < ARRAY_SIZE; i++) {
        sorted[i]  = (int16_t)i;
        reverse[i] = (int16_t)(ARRAY_SIZE - 1 - i);
        random_arr[i] = (int16_t)((i * 37 + 13) % ARRAY_SIZE);
    }

    uint32_t best, worst, sum, cycles;
    best = UINT32_MAX; worst = 0; sum = 0;

    /* --- Bubble sort WCET --- */
    uart_printf("\r\n=== bubble_sort(%d elements) ===\r\n", ARRAY_SIZE);

    /* Best case: already sorted */
    memcpy(arr, sorted, sizeof(sorted));
    DWT_START();
    bubble_sort(arr, ARRAY_SIZE);
    DWT_STOP();
    uart_printf("  sorted input (best):   %lu cycles  (%.2f us)\r\n",
                _dwt_elapsed, _dwt_elapsed / 150.0f);

    /* Worst case: reverse sorted */
    for (int r = 0; r < NUM_RUNS; r++) {
        memcpy(arr, reverse, sizeof(reverse));
        DWT_START();
        bubble_sort(arr, ARRAY_SIZE);
        DWT_STOP();
        cycles = _dwt_elapsed;
        if (cycles < best) best = cycles;
        if (cycles > worst) worst = cycles;
        sum += cycles;
    }
    uart_printf("  reverse-sorted (%d runs): best=%lu  worst=%lu  avg=%lu cycles\r\n",
                NUM_RUNS, best, worst, sum / NUM_RUNS);
    uart_printf("  WCET with 20%% margin:    %lu cycles  (%.2f us)\r\n",
                (uint32_t)(worst * 1.2f), worst * 1.2f / 150.0f);

    /* --- FIR filter WCET --- */
    uart_printf("\r\n=== fir_filter(16 taps) ===\r\n");
    best = UINT32_MAX; worst = 0; sum = 0;
    for (int r = 0; r < NUM_RUNS; r++) {
        DWT_START();
        volatile int32_t result = fir_filter(random_arr, fir_h, 16);
        (void)result;
        DWT_STOP();
        cycles = _dwt_elapsed;
        if (cycles < best) best = cycles;
        if (cycles > worst) worst = cycles;
        sum += cycles;
    }
    uart_printf("  best=%lu  worst=%lu  avg=%lu cycles\r\n", best, worst, sum/NUM_RUNS);
    uart_printf("  WCET with 20%% margin:    %lu cycles  (%.2f us)\r\n",
                (uint32_t)(worst * 1.2f), worst * 1.2f / 150.0f);

    uart_printf("\r\nMeasurement complete.\r\n");
    vTaskDelete(NULL);
}
```

### Step 4 — Build, flash, and record results

```bash
make setup && make && make flash
```

Copy the full terminal output into a table in your report.

**Checkpoint A:** Terminal output with all three function measurements.

> **Question A1:** Bubble sort has the same code path for all reverse-sorted inputs. Why does `worst` vary across the 1 000 runs even with identical input? Name two hardware effects that cause this variability.

> **Question A2:** The FIR filter is data-independent (no branches). Does it show variability across runs? What is the ratio of worst to best cycles? Explain.

> **Question A3:** You will use `bubble_sort` as a task in Lab 07. Its period is T = 50 ms. Using the WCET + 20% margin as $C_i$: compute the utilisation $C_i / T_i$. At what maximum number of identical `bubble_sort` tasks does the total utilisation exceed the Liu & Layland bound for RMS?

---

## Part B — Interrupt Latency Measurement

### Step 1 — Set up the GPIO toggling ISR

Wire a jumper from `P1_0` (output, driven by signal generator or a software toggle task) to `P1_1` (input, interrupt on rising edge). The ISR immediately toggles `P1_2` (output to oscilloscope probe).

```c
/* latency_test.c */
void GPIO1_IRQHandler(void)
{
    GPIO_ClearPinsInterruptFlags(GPIO1, 1u << 1);
    GPIO_TogglePinsOutput(GPIO1, 1u << 2);  /* output as fast as possible */
}
```

### Step 2 — Configure GPIO and NVIC

```c
/* Input: P1_1 rising edge interrupt */
gpio_pin_config_t in_cfg  = { kGPIO_DigitalInput,  0, kGPIO_IntRisingEdge };
GPIO_PinInit(GPIO1, 1, &in_cfg);
GPIO_EnableInterrupts(GPIO1, 1u << 1);
EnableIRQ(GPIO1_IRQn);

/* Output: P1_2 */
gpio_pin_config_t out_cfg = { kGPIO_DigitalOutput, 0, kGPIO_NoIntmode };
GPIO_PinInit(GPIO1, 2, &out_cfg);
```

### Step 3 — Drive the input and measure

**Option A — Logic analyser:** set `P1_0` toggling at 1 kHz from a software task; measure `P1_1`-rising to `P1_2`-rising on the analyser.

**Option B — Oscilloscope:** probe `P1_1` (CH1) and `P1_2` (CH2); use rising-edge trigger on CH1; measure propagation delay.

**Option C — DWT inside the ISR:**

```c
void GPIO1_IRQHandler(void)
{
    uint32_t t_isr = DWT_GetCycles();   /* first thing in ISR */
    GPIO_ClearPinsInterruptFlags(GPIO1, 1u << 1);
    /* t_isr - t_gpio_assert = cycles from IRQ line assertion to here */
    latency_buffer[latency_idx++ % 64] = t_isr - last_gpio_toggle;
    GPIO_TogglePinsOutput(GPIO1, 1u << 2);
}
```

Where `last_gpio_toggle` is set just before toggling `P1_0` in the generating task.

### Step 4 — Measure with and without critical section

Repeat the measurement with a FreeRTOS critical section active in a background task:

```c
void vCriticalSectionTask(void *pv)
{
    for (;;) {
        taskENTER_CRITICAL();
        /* Simulate a long critical section — ISR is masked during this time */
        volatile uint32_t i = 0;
        while (i++ < 150000) {}   /* ~1 ms at 150 MHz */
        taskEXIT_CRITICAL();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
```

**Checkpoint B:** Table or oscilloscope screenshot showing latency with and without the critical section.

> **Question B1:** Your measured best-case latency should be 12–18 cycles. Convert this to nanoseconds at 150 MHz. How does this compare to the theoretical minimum from the ARM Cortex-M33 TRM (exception entry stacking)?

> **Question B2:** With the critical section active, what is the worst-case interrupt latency? Where does the extra latency come from? What FreeRTOS configuration parameter controls this?

> **Question B3:** The ISR priority is set to `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY - 1`. Why can this ISR still be deferred by `taskENTER_CRITICAL`? What priority would you set to make the ISR **non-maskable** by FreeRTOS?

---

## Part C — Full Task WCET in Context

### Step 1 — Measure task response time end-to-end

Create a periodic task that runs `bubble_sort` on random data every 50 ms. Use DWT to measure from task entry to task return (including OS overhead at context switch):

```c
void vSortTask(void *pv)
{
    static int16_t arr[ARRAY_SIZE];
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(50));
        /* Prepare random input */
        for (int i = 0; i < ARRAY_SIZE; i++) arr[i] = (int16_t)(rand() % 1000);
        DWT_START();
        bubble_sort(arr, ARRAY_SIZE);
        DWT_STOP();
        uart_printf("[SORT] %lu cycles  (%.2f us)\r\n",
                    _dwt_elapsed, _dwt_elapsed / 150.0f);
    }
}
```

Run with two background tasks also active (prio 1 and 2). Compare the measured cycles to Part A's isolated measurement.

> **Question C1:** Does running alongside other tasks increase the measured execution time? What hardware effect causes this? What does this tell you about measurement-based WCET methodology?

---

## Deliverables

| # | Item |
|---|------|
| 1 | Part A terminal output — full measurement table for all three functions |
| 2 | Part A analysis — WCET + 20% margin for `bubble_sort` and schedulability calculation (Question A3) |
| 3 | Part B measurement — latency table or oscilloscope screenshot |
| 4 | Part B analysis — with and without critical section (Question B2) |
| 5 | Written answers to Questions A1–A3, B1–B3, C1 |
| 6 | `dwt.h` header file |

---

## Key Concepts Demonstrated

| Concept | Where you saw it |
|---------|-----------------|
| DWT cycle counter enable and read | Part A setup |
| Best/average/worst-case measurement methodology | Part A |
| WCET safety margin application | Part A Question A3 |
| Interrupt latency: hardware stacking overhead | Part B |
| BASEPRI masking and FreeRTOS critical section | Part B |
| Context and cache effects on WCET | Part C |

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| Wilhelm et al. (2008) | "The worst-case execution time problem" (*ACM TECS*) |
| ARM Cortex-M33 TRM | §DWT, §B2.3 exception entry |
| NXP MCXN236 RM | §CoreDebug, §DWT |
| FreeRTOS docs | `taskENTER_CRITICAL`, `configMAX_SYSCALL_INTERRUPT_PRIORITY` |
