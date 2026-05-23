---
theme: seriph
background: https://cover.sli.dev
title: "RTOS · Week 13 — Low-Power RTOS & Tickless Idle"
info: |
  ## Real-Time Operating Systems
  Week 13 — Low-Power RTOS & Tickless Idle

  KMUTNB · Faculty of Engineering · Department of Electrical and Computer Engineering
  Master of Engineering (M.Eng.) — Electrical and Computer Engineering
class: text-center
transition: slide-left
mdc: true
routeAlias: week-13
fonts:
  sans: Inter
  serif: Lora
  mono: Fira Code
---

# Real-Time Operating Systems

## Week 13 — Low-Power RTOS & Tickless Idle

ARM power states · configUSE_TICKLESS_IDLE · MCXN236 power domains

<div class="pt-10 opacity-70 text-sm">
  KMUTNB · Faculty of Engineering · M.Eng. in Electrical & Computer Engineering
</div>

<div class="abs-br m-6 text-xs opacity-50">
  Reading: FreeRTOS Tickless Idle docs · NXP MCXN236 RM §PMC
</div>

---
layout: two-cols
layoutClass: gap-8
---

# From Week 12 to Week 13

We can now scale across multiple cores for performance.

But battery-powered IoT devices have the opposite constraint — every microjoule counts. Running the CPU at full speed while waiting for events wastes power.

How does an RTOS cooperate with hardware power management to slash idle current?

::right::

<div class="mt-10 px-5 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-sm leading-relaxed">

**This week — power-aware RTOS design:**

- ARM Cortex-M power states: Run, Sleep, Deep Sleep
- The tick interrupt problem: wakeup every 1 ms even when idle
- FreeRTOS tickless idle: suppress ticks during long sleeps
- MCXN236 power domains and clock gating
- `vPortSuppressTicksAndSleep` implementation
- Lab 10: measure current with and without tickless idle

</div>

---

# Week 13 — Learning Objectives

By the end of this lecture you will be able to:

<v-clicks>

- **Describe** ARM Cortex-M power states and the WFI/WFE instructions.
- **Explain** why the RTOS tick interrupt prevents deep sleep and how tickless idle solves it.
- **Configure** FreeRTOS tickless idle with `configUSE_TICKLESS_IDLE`.
- **Implement** `vPortSuppressTicksAndSleep` for the MCXN236 low-power timer (LPTMR).
- **Identify** MCXN236 power domains and select the appropriate power mode per task.

</v-clicks>

<div v-click class="mt-6 px-4 py-2 border-l-4 border-amber-500 bg-amber-50 dark:bg-amber-900/20 text-sm">
Maps to <b>CLO 4 &amp; CLO 5</b> — analyse resource usage and evaluate platform constraints for embedded RTOS applications.
</div>

---
layout: section
---

# Part 1
## ARM Cortex-M Power States

---

# Cortex-M33 Power Modes

<div class="mt-4 text-sm">

| Mode | CPU | Clocks | SRAM | Wake latency | Current |
|------|-----|--------|------|-------------|---------|
| **Run** | Active | Full | Retained | — | ~15 mA |
| **Sleep** (WFI) | Halted | Peripheral clocks ON | Retained | 1 cycle | ~5 mA |
| **Deep Sleep** (DEEPSLEEP) | Halted | Most clocks OFF | Retained | ~10 µs | ~200 µA |
| **Power Down** | Off | Off | Selectable | ~1 ms | ~5 µA |
| **Deep Power Down** | Off | Off | Lost | ~5 ms | ~1 µA |

</div>

```c
/* Enter Sleep: CPU halted, wake on any interrupt */
__WFI();   /* Wait For Interrupt */

/* Enter Deep Sleep: set SLEEPDEEP bit first */
SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
__DSB();
__WFI();
SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;  /* clear after wake */
```

<div v-click class="mt-3 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
For an RTOS: Sleep is safe (wake on any interrupt including tick). Deep Sleep requires suppressing the tick timer and using a low-power wakeup timer instead.
</div>

---
layout: section
---

# Part 2
## The Tick Interrupt Problem

---
layout: two-cols
layoutClass: gap-6
---

# Why the Tick Wastes Power

FreeRTOS generates a SysTick interrupt every `1/configTICK_RATE_HZ` seconds (typically 1 ms).

Even when **all tasks are blocked** (waiting for timers or events), the CPU must wake every 1 ms to run the tick handler.

```
Timeline (all tasks blocked for 500 ms):

  ───█─█─█─█─█─█─█─ ··· ─█─█─█─██ 
     ↑ ↑ ↑ ↑                     ↑
  tick ISR (1ms)           task unblocks
  CPU wakes 500 times
  No useful work done
```

At 1 mA per wake event × 500 events/s: average current is much higher than necessary.

::right::

<div class="mt-6 px-5 py-4 rounded-lg bg-green-50 dark:bg-green-900/30 text-sm leading-relaxed">

### Tickless idle solution

When the scheduler finds all tasks blocked:

1. Determine time until next task unblocks: **idlePeriod**
2. Reprogram a low-power wakeup timer to fire at **idlePeriod**
3. Stop the SysTick, enter Deep Sleep
4. On wakeup, correct the tick count by **idlePeriod**
5. Resume normal tick operation

```
  ──── DEEP SLEEP ─────────────────█
  ↑ tick suppressed                ↑
  all tasks blocked          LPTMR fires,
                             tick corrected
```

Result: CPU stays in Deep Sleep for the full idle period — potentially hundreds of milliseconds.

</div>

---
layout: section
---

# Part 3
## FreeRTOS Tickless Idle

---

# Configuring Tickless Idle

```c
/* FreeRTOSConfig.h */
#define configUSE_TICKLESS_IDLE    1   /* built-in port support */
/* or */
#define configUSE_TICKLESS_IDLE    2   /* custom implementation */

/* Minimum idle period before entering tickless (in ticks) */
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP   2
```

**Built-in port (`= 1`):** uses SysTick reload trick — SysTick is reprogrammed to count the idle period.

**Custom (`= 2`):** you implement `vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime)`.

```c
/* Hook called by FreeRTOS idle task — you implement this */
void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime)
{
    /* 1. Disable SysTick */
    /* 2. Program LPTMR for xExpectedIdleTime ticks */
    /* 3. Enter Deep Sleep (WFI with SLEEPDEEP) */
    /* 4. On wake: read elapsed ticks from LPTMR */
    /* 5. Correct FreeRTOS tick count: vTaskStepTick(elapsed) */
    /* 6. Re-enable SysTick */
}
```

---

# vPortSuppressTicksAndSleep on MCXN236

```c {all|1-12|14-28}{maxHeight:'350px'}
void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime)
{
    /* Disable SysTick before sleep */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    /* Program LPTMR (32 kHz RTC clock) */
    uint32_t lptmr_ticks = (xExpectedIdleTime * 32768UL)
                           / configTICK_RATE_HZ;
    LPTMR0->CMR = lptmr_ticks;
    LPTMR0->CSR = LPTMR_CSR_TEN_MASK | LPTMR_CSR_TIE_MASK;

    /* Enter Deep Sleep */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    /* Wake: determine how long we actually slept */
    LPTMR0->CSR &= ~LPTMR_CSR_TEN_MASK;  /* stop LPTMR */
    uint32_t elapsed_lptmr = LPTMR0->CNR;
    TickType_t elapsed_ticks = (elapsed_lptmr * configTICK_RATE_HZ)
                               / 32768UL;

    /* Correct FreeRTOS tick count */
    vTaskStepTick(elapsed_ticks);

    /* Re-enable SysTick */
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}
```

---
layout: section
---

# Part 4
## MCXN236 Power Domains

---

# MCXN236 Power Architecture

<div class="mt-4 text-sm">

| Domain | Contents | Can power off? |
|--------|---------|---------------|
| **VDDCORE** | Both CM33 cores, SRAM, AHB fabric | No (retention only) |
| **VDDA** | ADC, DAC, analog comparators | Yes |
| **VDDIO** | GPIO pads, I/O ring | Partial (per-bank) |
| **VBAT** | RTC, LPTMR, tamper detect | No (always on) |

</div>

```c
/* Clock gate a peripheral when not in use */
CLOCK_DisableClock(kCLOCK_Lpuart0);   /* gate UART0 clock */

/* Re-enable on demand */
CLOCK_EnableClock(kCLOCK_Lpuart0);

/* Enter Low-Power Run mode (reduce core voltage + freq) */
POWER_SetRunPowerMode(kPOWER_ModeLP);  /* ~50 MHz max */
```

<div v-click class="mt-4 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
Strategy: use FreeRTOS tickless idle for idle periods. Use clock gating in task-level code to disable peripherals between uses. Use Low-Power Run mode for sustained low-activity periods.
</div>

---
layout: section
---

# Part 5
## Power-Aware Task Design

---
layout: two-cols
layoutClass: gap-6
---

# Design Patterns for Low Power

**Pattern 1 — Block, don't poll:**

```c
/* BAD: spin loop burns CPU */
while (!sensor_ready()) {}
read_sensor();

/* GOOD: block on semaphore, let idle run */
xSemaphoreTake(xSensorReady, portMAX_DELAY);
read_sensor();
```

**Pattern 2 — Coalesce wakeups:**

```c
/* BAD: 10 tasks wake at different ms offsets */
vTaskDelay(pdMS_TO_TICKS(10));  /* task A */
vTaskDelay(pdMS_TO_TICKS(11));  /* task B — separate wakeup */

/* GOOD: align periods to a common base */
vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(10));
/* All 10-ms tasks wake together → single tick */
```

::right::

<div class="mt-6 px-5 py-4 rounded-lg bg-amber-50 dark:bg-amber-900/30 text-sm leading-relaxed">

**Pattern 3 — Peripheral power gating in tasks:**

```c
void vADCTask(void *pv) {
    for (;;) {
        xSemaphoreTake(xADCRequest, portMAX_DELAY);

        CLOCK_EnableClock(kCLOCK_Adc0);
        ADC_DoConversion(&result);
        CLOCK_DisableClock(kCLOCK_Adc0);

        xQueueSend(xResultQ, &result, 0);
    }
}
```

ADC clock is only on during the conversion — not during the (potentially long) wait between requests.

<div class="mt-3 text-xs opacity-70">
Measure with a current probe or NXP MCU-Link power measurement: compare average current with and without these patterns.
</div>

</div>

---
layout: section
---

# Part 6
## Lab 10 — Tickless Idle Measurement

---
layout: two-cols
layoutClass: gap-6
---

# Lab 10 — Measure the Difference

<v-clicks>

**Step 1 — Baseline (tick always on):**
1. Create one task that samples ADC every 500 ms, blocks between samples
2. `configUSE_TICKLESS_IDLE = 0`
3. Measure average current with MCU-Link power monitor or current probe

**Step 2 — Tickless idle:**
4. Set `configUSE_TICKLESS_IDLE = 1`
5. Re-run same task, same 500 ms period
6. Measure average current again

**Step 3 — Custom LPTMR implementation:**
7. Set `configUSE_TICKLESS_IDLE = 2`
8. Implement `vPortSuppressTicksAndSleep` using LPTMR0
9. Verify tick correction accuracy with a 1-second LED toggle

</v-clicks>

::right::

<div class="mt-8 px-5 py-4 rounded-lg bg-amber-50 dark:bg-amber-900/30 text-sm leading-relaxed">

**What to submit**

- Current measurements: baseline vs tickless (mA, ideally with waveform screenshot)
- `vPortSuppressTicksAndSleep` implementation
- LED toggle accuracy: expected 1000 ms, actual (drift over 60 s)?
- Analysis: how much current saving per mA·h over 24 hours?

<div class="mt-3 text-xs opacity-70">
Reading — FreeRTOS tickless idle documentation · NXP MCXN236 RM §PMC, §LPTMR
</div>

</div>

---
layout: default
---

# Key Takeaways

<v-clicks>

- Cortex-M33 has multiple power states: **Sleep** (wake in 1 cycle, peripheral clocks on) and **Deep Sleep** (most clocks off, µA-level current, µs wake latency).
- Without tickless idle, the RTOS tick interrupt wakes the CPU every 1 ms — even when all tasks are blocked. For a 500 ms idle period this is **500 unnecessary wakeups**.
- **FreeRTOS tickless idle** (`configUSE_TICKLESS_IDLE = 1 or 2`) suppresses the tick, programs a low-power wakeup timer, enters Deep Sleep, then corrects the tick count on wake with `vTaskStepTick`.
- **MCXN236**: use the LPTMR (clocked from 32 kHz VBAT domain) as the wakeup timer — it survives Deep Sleep where SysTick does not.
- Power-aware task design: **block don't poll**, **coalesce wakeups**, **gate peripheral clocks** between uses. These patterns combine with tickless idle for maximum battery life.

</v-clicks>

<div v-click class="mt-5 text-center text-base px-4 py-2 rounded bg-blue-100 dark:bg-blue-900/40">
Next — <b>Week 14: Zephyr RTOS</b> — a modern, Kconfig-based alternative to FreeRTOS.
</div>

---

# Before Next Week

<div class="grid grid-cols-2 gap-8 mt-6">

<div>

### Reading
- **FreeRTOS** — Tickless Idle Mode documentation (freertos.org)
- **NXP MCXN236 RM** — Chapter PMC (Power Management Controller), LPTMR chapter
- Review: ARM Cortex-M33 TRM §B2.5 (power management)

### Lab
- Complete **Lab 10** — tickless idle implementation and measurement
- Try adding clock gating to the ADC task
- Report total energy saving over 1 hour

</div>

<div>

### Check yourself
<div class="text-sm">

1. All FreeRTOS tasks are blocked for 200 ms. Without tickless idle, how many SysTick interrupts fire? (assume 1 kHz tick)
2. `vTaskStepTick(elapsed)` is called with an argument that is 2 ticks too small. What symptom does this cause?
3. The LPTMR runs at 32.768 kHz. What is the maximum tickless sleep duration if the LPTMR counter is 16-bit?
4. A task calls `CLOCK_DisableClock(kCLOCK_Adc0)` and then goes back to its blocking wait. The idle task then enters Deep Sleep. When the task wakes and tries to use the ADC, what must it do first?

</div>

</div>

</div>

---
layout: end
class: text-center
---

# Week 13 Complete

Low-Power RTOS & Tickless Idle

<div class="mt-4 text-sm opacity-70">
Real-Time Operating Systems · KMUTNB · M.Eng. ECE<br/>
Next — Week 14 · Zephyr RTOS
</div>

<style>
:root { --slidev-theme-primary: #003874; }
.slidev-layout h1 { color: #003874; }
.dark .slidev-layout h1 { color: #7ba7d9; }
table { font-size: 0.92em; }
</style>
