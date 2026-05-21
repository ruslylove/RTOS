# Lab 01 — Toolchain & Environment Verification

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** NXP FRDM-MCXN236 (Cortex-M33)  
**Estimated time:** 2–3 hours  

---

## Objectives

By completing this lab you will be able to:

1. Build and flash a bare-metal FreeRTOS project using the ARM cross-compiler toolchain.
2. Verify that FreeRTOS creates and schedules tasks correctly by reading UART output.
3. Integrate SEGGER SystemView into a FreeRTOS project.
4. Capture a real-time trace and identify task context switches on the timeline.
5. Observe how task priority affects scheduling — the foundation for everything in Module 2.

---

## Prerequisites

Complete the **Lab 1 Setup Guide** (`lab01_setup_guide.md`) before starting:

- [ ] ARM GNU Toolchain installed and `arm-none-eabi-gcc` is on `PATH` (or toolchain path set in `Makefile`)
- [ ] `pyocd` installed (`pip install pyocd`)
- [ ] FRDM-MCXN236 board connected via the **MCU-LINK USB** port
- [ ] MCU-Link firmware **re-flashed to J-Link** (Step 3 of setup guide)
- [ ] SEGGER SystemView application installed on your laptop

---

## Project Structure

```
lab01_setup_verify/
├── Makefile                  ← build, flash, and debug targets
├── linker_MCXN236.ld         ← flash/RAM memory map
├── src/
│   ├── startup_MCXN236.s     ← vector table + Reset_Handler
│   ├── main.c                ← hw_init, tasks, scheduler start
│   ├── FreeRTOSConfig.h      ← kernel configuration
│   ├── uart.h / uart.c       ← bare-metal LPUART4 driver
│   └── SystemView/           ← (created in Part C — you add these)
└── FreeRTOS-Kernel/          ← (cloned in Part A)
```

### What the code does

Two FreeRTOS tasks run in this lab:

| Task | Priority | Period | What it does |
|------|----------|--------|--------------|
| `vBlinkTask` | 2 (HIGH) | 500 ms | Prints `LED ON` / `LED OFF` toggle count |
| `vHeartbeatTask` | 1 (LOW) | 1000 ms | Prints the FreeRTOS tick uptime in ms |

A shared **UART mutex** (`xUartMutex`) prevents the two tasks from printing at the same time.  
This is the simplest possible demonstration of priority-based preemptive scheduling.

---

## Part A — Build and Flash

### Step 1 — Clone FreeRTOS-Kernel

```bash
cd labs/lab01_setup_verify
make setup
```

This runs `git clone --depth=1` to fetch the FreeRTOS kernel source into `FreeRTOS-Kernel/`.  
You only need to do this once.

### Step 2 — Build

```bash
make
```

Expected output:
```
arm-none-eabi-gcc  ...  -o lab01.elf
arm-none-eabi-objcopy -O binary lab01.elf lab01.bin
arm-none-eabi-size lab01.elf
   text    data     bss     dec     hex filename
  12xxx     xxx   xxxxx   xxxxx    xxxx lab01.elf
```

> **Troubleshooting:** If `arm-none-eabi-gcc: not found`, either add the toolchain to `PATH`
> or set the path at the top of `Makefile`:
> ```makefile
> TOOLCHAIN ?= /path/to/arm-none-eabi/bin
> ```

### Step 3 — Flash

Connect the board, then:

```bash
make flash
```

pyocd will erase, program, and reset the MCU. The board starts running immediately.

---

## Part B — UART Output Verification

Open a serial terminal on your laptop. The board exposes a CDC-ACM virtual COM port over the same USB cable.

**macOS / Linux:**
```bash
# find the port first
ls /dev/tty.usbmodem*   # or /dev/ttyACM*

# open at 115200 baud
screen /dev/tty.usbmodemXXXX 115200
# or: minicom -b 115200 -D /dev/tty.usbmodemXXXX
```

**Windows:** Use PuTTY or Tera Term — `COMx`, 115200 8N1.

### Expected output

```
=== RTOS Lab 01: Toolchain & Environment Verify ===
Tasks: Blink (500 ms) | Heartbeat (1000 ms)
SystemView: disabled (rebuild with: make SYSVIEW=1)

[MAIN] Starting scheduler...

[BLINK] LED ON   (toggle #1)
[BLINK] LED OFF  (toggle #2)
[HB]    uptime = 1000 ms
[BLINK] LED ON   (toggle #3)
[BLINK] LED OFF  (toggle #4)
[HB]    uptime = 2000 ms
...
```

> **Key observations:**
> - `[BLINK]` prints twice for every one `[HB]` print — reflecting 500 ms vs. 1000 ms periods.
> - Uptime increments by exactly 1000 ms each line — the FreeRTOS tick is running correctly.

**Checkpoint A:** Show a terminal capture of at least 5 seconds of output to your instructor.

---

## Part C — SEGGER SystemView Integration

The setup guide (Step 4) explains SystemView in detail. Follow those steps to add the source files to `src/SystemView/`, then rebuild with the `SYSVIEW=1` flag.

### Step 1 — Copy SystemView source files

Locate your SystemView installation. On macOS it is typically:
```
/Applications/SEGGER/SystemView_V3xx/Src/
```

Create the folder and copy these files:
```
src/SystemView/
  SEGGER_SYSVIEW.c
  SEGGER_SYSVIEW.h
  SEGGER_SYSVIEW_ConfDefaults.h
  SEGGER_SYSVIEW_Int.h
  SEGGER_RTT.c
  SEGGER_RTT.h
  SEGGER_RTT_Conf.h
  SEGGER_RTT_printf.c
  Config/SEGGER_SYSVIEW_Config_FreeRTOS.c
  Sample/OS/SEGGER_SYSVIEW_FreeRTOS.h
```

### Step 2 — Configure the SystemView target descriptor

Open `src/SystemView/SEGGER_SYSVIEW_Config_FreeRTOS.c` and update three lines:

```c
#define SYSVIEW_APP_NAME        "Lab 01 — Setup Verify"
#define SYSVIEW_DEVICE_NAME     "Cortex-M33"
#define SYSVIEW_RAM_BASE        0x20000000u
```

### Step 3 — Build with SystemView enabled

```bash
make sysview
# equivalent to: make SYSVIEW=1
```

The Makefile adds `-DUSE_SYSVIEW` and compiles the SystemView source files automatically.  
`main.c` calls `SEGGER_SYSVIEW_Conf()` before `vTaskStartScheduler()` when this flag is set.

### Step 4 — Capture a trace

1. Flash the new binary:
   ```bash
   make SYSVIEW=1 flash
   ```
2. Open **SEGGER SystemView** on your laptop.
3. Go to **Target → Start Recording**.
4. Enter connection parameters:
   - **Target Device:** `MCXN236`
   - **Interface:** `SWD`
   - **Speed:** `Auto`
5. Click **OK**.

You should see the real-time Gantt chart with `Blink` and `HB` tasks switching every 500 ms and 1000 ms.

**Checkpoint B:** Screenshot the SystemView timeline showing both tasks and at least 2 seconds of recording.

---

## Part D — Experiments

Work through these experiments in order. For each one, record what you observe (UART output or SystemView screenshot) and answer the question below it.

---

### Experiment 1 — Priority swap

In `main.c`, swap the task priorities:

```c
xTaskCreate(vBlinkTask,     "Blink", configMINIMAL_STACK_SIZE, NULL, 1, NULL); /* was 2 */
xTaskCreate(vHeartbeatTask, "HB",    configMINIMAL_STACK_SIZE, NULL, 2, NULL); /* was 1 */
```

Rebuild, flash, and observe.

> **Question 1:** Does the output order or timing change? Why or why not?  
> *(Hint: both tasks spend almost all their time blocked in `vTaskDelay`. Think about what happens when both unblock at the same tick.)*

---

### Experiment 2 — Equal priorities

Set both tasks to the same priority:

```c
xTaskCreate(vBlinkTask,     "Blink", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
xTaskCreate(vHeartbeatTask, "HB",    configMINIMAL_STACK_SIZE, NULL, 1, NULL);
```

> **Question 2:** What scheduling policy does FreeRTOS apply when two tasks have equal priority and are both ready? Where is this configured in `FreeRTOSConfig.h`?

---

### Experiment 3 — Periods and CPU load

Change `vBlinkTask`'s period from 500 ms to 50 ms (keep Heartbeat at 1000 ms, restore original priorities).

```c
vTaskDelay(pdMS_TO_TICKS(50));
```

Rebuild and capture a SystemView trace.

> **Question 3:** Open the **CPU Load** view in SystemView (View → CPU Load). What fraction of CPU time does `Blink` now consume?  
> Compute it by hand using the utilization formula: $U = C/T$.  
> Do the measured and computed values agree? Explain any discrepancy.

---

### Experiment 4 — Remove the mutex (observe the problem)

In `vBlinkTask` and `vHeartbeatTask`, comment out the `xSemaphoreTake` / `xSemaphoreGive` calls (keep the `uart_printf` lines). Use Experiment 3's short period (50 ms Blink) to make interleaving more likely.

> **Question 4:** Do you observe corrupted or interleaved output on the terminal?  
> Why is this a **race condition** even on a single-core microcontroller?  
> What category of kernel primitive fixes it, and what does the mutex protect here?

Restore the mutex calls before continuing.

---

## Deliverables

Submit the following to the course LMS by the deadline:

| # | Item |
|---|------|
| 1 | Terminal capture (5+ seconds) showing correct `[BLINK]` / `[HB]` output |
| 2 | SystemView screenshot — timeline with both tasks, at least 2 seconds |
| 3 | Written answers to Questions 1–4 (a few sentences each) |
| 4 | Modified `main.c` from Experiment 3 (50 ms Blink, with mutex restored) |

---

## Key Concepts Introduced

| Concept | Where you saw it |
|---------|-----------------|
| Preemptive, priority-based scheduling | Priority swap (Exp 1) |
| Time-slicing among equal-priority tasks | Equal priority (Exp 2) |
| CPU utilization $U = C/T$ | CPU Load view (Exp 3) |
| Race condition on shared resource | Mutex removal (Exp 4) |
| Mutex as a critical-section guard | `xUartMutex` throughout |

These are the building blocks for Module 2 (scheduling theory) and Module 3 (synchronisation).

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| Barry, *Mastering the FreeRTOS Kernel* | Ch. 1 (tasks), Ch. 3 (queue/semaphore basics) |
| Laplante, *Real-Time Systems Design* | Ch. 1–2 (task model, deadline classes) |
| FreeRTOS docs | `xTaskCreate`, `vTaskDelay`, `xSemaphoreCreateMutex` |
| SEGGER SystemView User Guide | Recording setup, CPU Load view |
