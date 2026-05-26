# Lab 02 — FreeRTOS Basics: Tasks, Queues, Semaphores & Timers

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** NXP FRDM-MCXN236 (Cortex-M33, 150 MHz)  
**Estimated time:** 2–3 hours

---

## Objectives

By completing this lab you will be able to:

1. Build a multi-task FreeRTOS application that uses **four** kernel primitives — task, queue, semaphore, and software timer.
2. Explain the **producer–consumer** pattern and observe what happens when a queue overflows.
3. Use a **mutex** to protect a shared resource and demonstrate the corruption that occurs without one.
4. Distinguish a **binary** semaphore from a **counting** semaphore by observing lost signals.
5. Configure a **software timer** (one-shot vs. auto-reload) and explain the rules for timer callbacks.
6. Contrast `vTaskDelay` and `vTaskDelayUntil` and measure timing drift.

---

## Prerequisites

You must have completed **Lab 01** before starting:

- [ ] ARM GNU Toolchain 12 installed; `make` succeeds in `lab01_setup_verify/`
- [ ] NXP MCUXpresso SDK at the path set in `SDK_ROOT` in `Makefile`
- [ ] **LinkServer** installed: `/Applications/LinkServer_26.3.123/LinkServer`
- [ ] FRDM-MCXN236 connected via the **MCU-LINK USB** port
- [ ] Python 3 + `pyserial` for serial monitoring (`pip3 install pyserial`)

> All observation in this lab is done over UART — no SEGGER SystemView required.

---

## Project Structure

```
lab02_freertos_basics/
├── Makefile                  ← build, flash, debug targets
├── src/
│   ├── main.c                ← BOARD_InitHardware, RTOS objects, scheduler start
│   ├── lab_tasks.c / .h      ← the three tasks + timer callback
│   └── FreeRTOSConfig.h      ← kernel configuration
└── <SDK_ROOT>/               ← board support, FreeRTOS kernel, drivers (external)
    ├── rtos/freertos/freertos-kernel-upstream/   ← FreeRTOS kernel
    ├── devices/MCX/MCXN/MCXN236/               ← device startup + linker script
    └── examples/_boards/frdmmcxn236/            ← board.h, clock_config, pin_mux
```

### What the code does — the "Sensor Data Pipeline"

Three tasks and one software timer cooperate through shared kernel objects:

| Task / Object | Priority | Trigger | What it does |
|---------------|----------|---------|--------------|
| `vSensorTask`  | 3 (HIGH) | 500 ms period (`vTaskDelayUntil`) | Generates a fake ADC reading, sends it to `xSensorQueue` |
| `vDisplayTask` | 2 (MED)  | Blocks on `xSensorQueue` | Receives a reading and prints it (under `xUartMutex`) |
| `vStatusTask`  | 1 (LOW)  | Blocks on `xStatusSem` | Prints uptime + queue depth (under `xUartMutex`) |
| `StatusTimer`  | — (timer task) | 1000 ms, auto-reload | Callback gives `xStatusSem` to wake `vStatusTask` |

**Data flow:**

```
                  xSensorQueue (depth 8)
   vSensorTask ──────────────────────────▶ vDisplayTask
    (prio 3)         SensorReading_t          (prio 2)
    500 ms                                        │
                                                  │ xUartMutex
   StatusTimer ──xStatusSem──▶ vStatusTask ───────┤
    (1000 ms)                    (prio 1)         ▼
                                                 UART
```

- **Queue** (`xSensorQueue`) decouples the producer (`Sensor`) from the consumer (`Display`).
- **Semaphore** (`xStatusSem`) is a pure *signal* — the timer signals, the task waits.
- **Mutex** (`xUartMutex`) guarantees only one task calls `PRINTF` at a time.

---

## Part A — Build and Flash

### Step 1 — Build

```bash
cd labs/lab02_freertos_basics
make
```

Expected output:

```
arm-none-eabi-gcc ... -o lab02.elf
arm-none-eabi-objcopy -O binary lab02.elf lab02.bin
   text    data     bss     dec     hex filename
  30576      20   24260   54856    d648 lab02.elf
```

> **Troubleshooting:** If `arm-none-eabi-gcc: not found`, edit `TOOLCHAIN` at the top of
> `Makefile`. If the SDK sources are not found, edit `SDK_ROOT`.

### Step 2 — Flash

```bash
make flash
```

This calls LinkServer to erase, program, and reset the MCU.
Look for `Finished writing Flash successfully` in the output.

> **Alternative — pyocd:** activate the venv at
> `/Users/rus/Development/nxp_test/.venv/bin/pyocd`, then run
> `pyocd flash lab02.elf -t mcxn236`.

---

## Part B — UART Output Verification

Find the serial port and open a terminal:

```bash
ls /dev/cu.usbmodem*          # find the port
screen /dev/cu.usbmodemXXXXXX 115200
# Press Ctrl-A then Ctrl-\ to quit
```

**Or with Python (clean capture):**

```python
python3 -c "
import serial, time, sys
s = serial.Serial('/dev/cu.usbmodemXXXXXX', 115200, timeout=0.1)
s.reset_input_buffer()
end = time.time() + 10
while time.time() < end:
    d = s.read(256)
    if d: sys.stdout.buffer.write(d); sys.stdout.buffer.flush()
s.close()
"
```

### Expected output

```
=== RTOS Lab 02: FreeRTOS Basics ===
Tasks: Sensor | Display | Status
Primitives: Queue | Mutex | Semaphore | Timer

[MAIN] Starting scheduler...

[DISPLAY] t=500 ms  value=7
[DISPLAY] t=1000 ms  value=14

[STATUS] uptime=1003 ms  queue_waiting=0
[DISPLAY] t=1500 ms  value=21
[DISPLAY] t=2000 ms  value=28

[STATUS] uptime=2003 ms  queue_waiting=0
...
```

> **Key observations:**
> - Two `[DISPLAY]` lines appear for every one `[STATUS]` line — 500 ms vs. 1000 ms.
> - `queue_waiting=0` — the consumer keeps up with the producer, queue stays empty.
> - The `value` field steps by 7 each reading (`raw = (raw + 7) % 101`).
> - Lines never interleave — the mutex serialises `PRINTF` calls.

**Checkpoint A:** Show a terminal capture of at least 5 seconds of correct output to your instructor before continuing.

---

## Part C — Experiments

For each experiment: make the edit, `make flash`, observe the UART, answer the question.  
**Restore the original code after each experiment unless told otherwise.**

---

### Experiment 1 — The queue as a producer–consumer buffer

Make the consumer slower than the producer. In `lab_tasks.c`, inside `vDisplayTask`, add a delay after the print:

```c
xSemaphoreGive(xUartMutex);
vTaskDelay(pdMS_TO_TICKS(900));   /* ADD: slow consumer */
```

Rebuild, flash, and watch the `queue_waiting` count in `[STATUS]` lines.

> **Question 1:**
> a) The producer makes 2 readings/s; the consumer now drains ~1.1/s. The queue depth is **8** — roughly how many seconds until you see `[SENSOR] Queue full — reading dropped!`?
> b) `xQueueSend` in `vSensorTask` uses a timeout of `0`. What would change if it used `portMAX_DELAY` instead?
> c) In one sentence: what problem does a queue solve that a shared global variable does not?

Restore `vDisplayTask` (remove the added `vTaskDelay`).

---

### Experiment 2 — The mutex and the race condition

Remove `xUartMutex` and force a collision.

1. In `lab_tasks.c`, comment out **all four** `xSemaphoreTake(xUartMutex, …)` and `xSemaphoreGive(xUartMutex)` calls in `vDisplayTask` and `vStatusTask` (keep the `PRINTF` lines).
2. Speed both up to make collisions frequent:
   - `vSensorTask`: change `xPeriod` to `pdMS_TO_TICKS(50)`.
   - `main.c`: change the timer period to `pdMS_TO_TICKS(100)`.

Rebuild, flash, and study the terminal.

> **Question 2:**
> a) Do you see garbled lines — a `[DISPLAY]` fragment spliced into a `[STATUS]` line?
> b) This is a **race condition**. Explain why it happens *even on a single-core MCU*. (Hint: `PRINTF` sends characters one at a time; the scheduler can preempt mid-string.)
> c) FreeRTOS mutexes implement **priority inheritance** (`configUSE_MUTEXES 1`), but binary semaphores do not. Why does that matter when a low-priority task holds a lock that a high-priority task wants?

Restore the mutex calls and the original periods.

---

### Experiment 3 — Binary vs. counting semaphore (lost signals)

`xStatusSem` is a **binary** semaphore — it holds at most one token. Extra `Give` calls before `Take` are discarded.

1. In `main.c`, speed up the timer: `pdMS_TO_TICKS(1000)` → `pdMS_TO_TICKS(200)`.
2. In `lab_tasks.c`, slow down `vStatusTask` by adding a delay after the print:
   ```c
   xSemaphoreGive(xUartMutex);
   vTaskDelay(pdMS_TO_TICKS(1000));   /* ADD */
   ```

Rebuild and flash. The timer fires ~5×/s, but `vStatusTask` can only process ~1×/s.

> **Question 3a:** How many `[STATUS]` lines appear per second? How many timer gives are being **lost**?

Now switch to a **counting** semaphore. In `main.c`:

```c
xStatusSem = xSemaphoreCreateCounting(10, 0);   /* was xSemaphoreCreateBinary() */
```

(`configUSE_COUNTING_SEMAPHORES` is already `1` in `FreeRTOSConfig.h`.) Rebuild and flash.

> **Question 3b:** With the counting semaphore, what happens when `vStatusTask` finally runs after its 1000 ms delay? When is a binary semaphore the *correct* choice and when do you need a counting one?

Restore the binary semaphore, the 1000 ms timer period, and remove the `vStatusTask` delay.

---

### Experiment 4 — Software timer: one-shot vs. auto-reload

Change `StatusTimer` from auto-reload to one-shot. In `main.c`:

```c
xTimerCreate("StatusTimer", pdMS_TO_TICKS(1000),
             pdFALSE,   /* was pdTRUE — now one-shot */
             NULL, vStatusTimerCallback);
```

Rebuild and flash.

> **Question 4:**
> a) How many `[STATUS]` lines appear now, and why?
> b) The callback runs in the **timer service task** (`configTIMER_TASK_PRIORITY = 4`). Why must a timer callback **never** call a blocking API like `vTaskDelay` or a `Take` with a long timeout?
> c) Name one way to make a one-shot timer fire repeatedly without using `pdTRUE`.

Restore `pdTRUE`.

---

### Challenge (optional) — `vTaskDelay` vs. `vTaskDelayUntil`

`vSensorTask` uses `vTaskDelayUntil` for a drift-free 500 ms period. Replace it with `vTaskDelay` and add a variable workload:

```c
/* Simulate variable processing time */
for (volatile int i = 0; i < 200000; i++) { }

vTaskDelay(pdMS_TO_TICKS(500));        /* was vTaskDelayUntil(...) */
```

Rebuild, flash, and look at the `t=` timestamps in `[DISPLAY]` lines.

> **Challenge:** With `vTaskDelay`, the period becomes *work time + 500 ms* and timestamps drift from clean multiples of 500. With `vTaskDelayUntil` they stay locked. Explain why — what does `vTaskDelayUntil` measure its delay *from* that `vTaskDelay` does not?

Restore the original `vTaskDelayUntil` code.

---

## Deliverables

| # | Item |
|---|------|
| 1 | Terminal capture (5+ seconds) of baseline output (Checkpoint A) |
| 2 | Terminal capture from Experiment 1 showing `Queue full` drop |
| 3 | Terminal capture from Experiment 2 showing garbled output |
| 4 | Terminal captures from Experiment 3 — before and after counting semaphore |
| 5 | Written answers to Questions 1–4 (a few sentences each) |
| 6 | Challenge answer, if attempted |

---

## Key Concepts Introduced

| Concept | Where you saw it |
|---------|------------------|
| Producer–consumer decoupling via a queue | Experiment 1 |
| Queue overflow and non-blocking `xQueueSend` | Experiment 1 |
| Race condition on a shared resource | Experiment 2 |
| Mutex as a critical-section guard; priority inheritance | Experiment 2 |
| Binary vs. counting semaphore; lost signals | Experiment 3 |
| Software timers: one-shot vs. auto-reload; callback rules | Experiment 4 |
| Drift-free periodic timing with `vTaskDelayUntil` | Challenge |

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| Barry, *Mastering the FreeRTOS Kernel* | Ch. 4 (queues), Ch. 6 (semaphores/mutexes), Ch. 5 (software timers) |
| Laplante, *Real-Time Systems Design* | Ch. 3 (intertask communication, synchronisation) |
| FreeRTOS docs | `xQueueCreate`, `xQueueSend`, `xQueueReceive`, `xSemaphoreCreateMutex`, `xSemaphoreCreateBinary`, `xSemaphoreCreateCounting`, `xTimerCreate` |
