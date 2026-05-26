# Lab 01 — Toolchain & Environment Verification

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** NXP FRDM-MCXN236 (Cortex-M33, 150 MHz)  
**Estimated time:** 2–3 hours  

---

## Objectives

By completing this lab you will be able to:

1. Build a FreeRTOS project using the NXP MCUXpresso SDK and ARM cross-compiler.
2. Flash firmware and monitor the board using LinkServer.
3. Verify that FreeRTOS creates and schedules tasks correctly via UART output and a physical LED.
4. Observe how task priority affects scheduling — the foundation for Module 2.
5. Identify and prevent a race condition on a shared resource using a mutex.

---

## Prerequisites

Before starting, confirm these are in place:

- [ ] ARM GNU Toolchain 12 installed: `arm-none-eabi-gcc` available at  
  `/opt/homebrew/Cellar/arm-gcc-bin@12/12.2.Rel1/bin` (or update `TOOLCHAIN` in `Makefile`)
- [ ] NXP MCUXpresso SDK extracted at  
  `/Users/<you>/Development/NXP_project/SDK/mcuxsdk/mcuxsdk` (update `SDK_ROOT` in `Makefile` if different)
- [ ] **LinkServer** installed: `/Applications/LinkServer_26.3.123/LinkServer` (or later version)
- [ ] FRDM-MCXN236 board connected via the **MCU-LINK USB** port
- [ ] Python 3 + `pyserial` for serial monitoring (`pip3 install pyserial`)

> **Note:** This lab uses **LinkServer** (NXP's native flash/debug tool) — not pyocd or J-Link.  
> The board does **not** need its MCU-Link firmware reflashed.

---

## Project Structure

```
lab01_setup_verify/
├── Makefile                  ← build, flash, debug targets
├── src/
│   ├── main.c                ← task definitions, LED init, scheduler start
│   └── FreeRTOSConfig.h      ← kernel configuration
└── <SDK_ROOT>/               ← board support, FreeRTOS kernel, drivers (external)
    ├── rtos/freertos/freertos-kernel-upstream/   ← FreeRTOS kernel
    ├── devices/MCX/MCXN/MCXN236/                ← device startup + linker script
    └── examples/_boards/frdmmcxn236/             ← board.h, clock_config, pin_mux
```

The build pulls all SDK sources directly from `SDK_ROOT` — there is nothing to clone or install separately.

### What the code does

Two FreeRTOS tasks run continuously after the scheduler starts:

| Task | Priority | Period | What it does |
|------|----------|--------|--------------|
| `vBlinkTask` | 2 (HIGH) | 500 ms | Toggles the **physical red LED** (PIO4_18) and prints `LED ON / OFF` |
| `vHeartbeatTask` | 1 (LOW) | 1000 ms | Prints FreeRTOS tick uptime in ms |

A shared **UART mutex** (`xUartMutex`) prevents both tasks from calling `PRINTF` at the same time.

**Hardware note:** The red LED is active-low on GPIO4 pin 18 (board pin N10).  
`LOGIC_LED_ON = 0` (GPIO low = LED on), `LOGIC_LED_OFF = 1` (GPIO high = LED off).  
`LED_RED_TOGGLE()` is a macro defined in the SDK's `board.h`.

---

## Part A — Build and Flash

### Step 1 — Build

```bash
cd labs/lab01_setup_verify
make
```

Expected output (sizes will vary slightly):
```
arm-none-eabi-gcc ... -o lab01.elf
arm-none-eabi-objcopy -O binary lab01.elf lab01.bin
arm-none-eabi-size lab01.elf
   text    data     bss     dec     hex filename
  31588      20   16060   47668    ba34 lab01.elf
```

> **Troubleshooting — toolchain not found:** Edit the `TOOLCHAIN` variable at the top of `Makefile`  
> to point to your `arm-none-eabi-gcc` installation directory.

> **Troubleshooting — SDK not found:** Edit `SDK_ROOT` in `Makefile` to match where you extracted the SDK.

### Step 2 — Flash

Connect the board via the MCU-LINK USB port, then:

```bash
make flash
```

This calls LinkServer to erase, program, and reset the MCU:
```
Finished writing Flash successfully.
```

> **Alternative — pyocd:** pyocd is available in a virtualenv at  
> `/Users/rus/Development/nxp_test/.venv/bin/pyocd`.  
> Activate it first, then run `pyocd flash lab01.elf -t mcxn236`.

The board resets and starts executing immediately after flashing.

---

## Part B — Verify: UART Output and LED

### Serial terminal

The board exposes a USB-CDC virtual COM port over the MCU-LINK USB cable.

**Find the port:**
```bash
ls /dev/cu.usbmodem*
# Example: /dev/cu.usbmodemLBAZMNNVFKEDG3
```

**Monitor with screen:**
```bash
screen /dev/cu.usbmodemXXXXXX 115200
# Press Ctrl-A then Ctrl-\ to quit screen
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

**Windows:** PuTTY or Tera Term — `COMx`, 115200, 8N1.

### Expected output

```
=== RTOS Lab 01: Toolchain & Environment Verify ===
SDK: BOARD_InitHardware + PRINTF  |  CPU: PLL 150 MHz
Tasks: Blink (500 ms) | Heartbeat (1000 ms)

[MAIN] Starting scheduler...

[BLINK] LED ON   (toggle #1)
[BLINK] LED OFF  (toggle #2)
[HB]    uptime = 1003 ms
[BLINK] LED ON   (toggle #3)
[BLINK] LED OFF  (toggle #4)
[HB]    uptime = 2007 ms
...
```

### What to observe

- `[BLINK]` prints **twice** for every one `[HB]` line — 500 ms vs. 1000 ms periods.
- Uptime increments by approximately 1000 ms per `[HB]` line.
- The **red LED** on the board blinks visibly at 2 Hz (on 500 ms, off 500 ms).
- Output never garbles — the mutex serialises both tasks' `PRINTF` calls.

**Checkpoint A:** Capture at least 5 seconds of terminal output and photograph the blinking LED.

---

## Part C — Experiments

Work through these in order. Record UART output for each one.

---

### Experiment 1 — Priority swap

In `main.c`, swap the task priorities:

```c
xTaskCreate(vBlinkTask,     "Blink", 512, NULL, 1, NULL); /* was 2 */
xTaskCreate(vHeartbeatTask, "HB",    512, NULL, 2, NULL); /* was 1 */
```

Rebuild (`make`), flash, and observe the serial output.

> **Question 1:** Does the output order or timing change? Why or why not?  
> *(Hint: both tasks spend almost all their time blocked in `vTaskDelay`. Think about what happens when both unblock at the same tick.)*

---

### Experiment 2 — Equal priorities

Set both tasks to the same priority:

```c
xTaskCreate(vBlinkTask,     "Blink", 512, NULL, 1, NULL);
xTaskCreate(vHeartbeatTask, "HB",    512, NULL, 1, NULL);
```

> **Question 2:** What scheduling policy does FreeRTOS apply when two tasks have equal priority and are both ready?  
> Where in `FreeRTOSConfig.h` is this policy enabled?

---

### Experiment 3 — Periods and CPU load

Restore original priorities. Change `vBlinkTask`'s delay from 500 ms to **50 ms**:

```c
vTaskDelay(pdMS_TO_TICKS(50));
```

Rebuild, flash, and watch the LED — it should now blink at 10 Hz.

> **Question 3:** Using the utilization formula $U = C/T$, estimate the CPU load of `vBlinkTask`.  
> $C$ ≈ time spent executing (very short — a `PRINTF` call),  $T$ = 50 ms.  
> Does the scheduler still meet the 1000 ms heartbeat deadline? Why?

---

### Experiment 4 — Remove the mutex (observe the race)

In both `vBlinkTask` and `vHeartbeatTask`, comment out the `xSemaphoreTake` / `xSemaphoreGive` lines around `PRINTF`. Keep the 50 ms Blink period from Experiment 3 to maximise interleaving.

```c
/* xSemaphoreTake(xUartMutex, portMAX_DELAY); */
PRINTF("[BLINK] LED %s  (toggle #%u)\r\n", state, (unsigned)count);
/* xSemaphoreGive(xUartMutex); */
```

Rebuild, flash, and watch the terminal.

> **Question 4:** Do you observe garbled or interleaved output?  
> Why is this a **race condition** even on a single-core MCU?  
> What property of a mutex prevents it?

Restore the mutex calls before submitting.

---

## Deliverables

| # | Item |
|---|------|
| 1 | Terminal capture (5+ seconds) — correct `[BLINK]` / `[HB]` output |
| 2 | Photo or video of the red LED blinking on the board |
| 3 | Written answers to Questions 1–4 (a few sentences each) |
| 4 | Modified `main.c` from Experiment 3 (50 ms Blink, mutex restored) |

---

## Key Concepts Introduced

| Concept | Where you saw it |
|---------|-----------------|
| Preemptive, priority-based scheduling | Priority swap (Exp 1) |
| Time-slicing among equal-priority tasks | Equal priority (Exp 2) |
| CPU utilization $U = C/T$ | Short-period Blink (Exp 3) |
| Race condition on a shared resource | Mutex removal (Exp 4) |
| Mutex as a critical-section guard | `xUartMutex` throughout |

---

## Implementation Notes

### Why `configRUN_FREERTOS_SECURE_ONLY 1`

The MCXN236 boots and runs entirely in the ARM **secure world**. The FreeRTOS ARM_CM33_NTZ port must know this to set the correct `EXC_RETURN` value when launching the first task:

| Config | EXC_RETURN | Bit[6] | Result |
|--------|-----------|--------|--------|
| `configRUN_FREERTOS_SECURE_ONLY 0` (default) | `0xFFFFFFBC` | 0 — non-secure | Immediate fault — CPU is actually in secure mode |
| `configRUN_FREERTOS_SECURE_ONLY 1` | `0xFFFFFFFD` | 1 — secure | First task starts correctly |

Without this setting the scheduler appears to start (the banner prints) but no tasks ever execute.

### Why the SDK FreeRTOS kernel

This lab uses the NXP SDK's `freertos-kernel-upstream` (inside `SDK_ROOT/rtos/freertos/`) rather than a standalone FreeRTOS-Kernel clone. The SDK kernel is the version NXP has tested against this chip and board BSP. A development-branch clone may have incompatible `portasm.c` changes that interact badly with the secure-world boot sequence.

### LED hardware

| Signal | GPIO | Pin | Board pad | Active level |
|--------|------|-----|-----------|-------------|
| Red LED | GPIO4 | 18 | N10 | Low (0 = ON) |
| Blue LED | GPIO4 | 17 | R9 | Low (0 = ON) |
| Green LED | GPIO4 | 19 | R10 | Low (0 = ON) |

All three share GPIO4. The pin mux is set to `kPORT_MuxAlt0` (GPIO function) in `main()` before the scheduler starts.

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| Barry, *Mastering the FreeRTOS Kernel* | Ch. 1 (tasks), Ch. 3 (queue/semaphore basics) |
| Laplante, *Real-Time Systems Design* | Ch. 1–2 (task model, deadline classes) |
| FreeRTOS docs | `xTaskCreate`, `vTaskDelay`, `xSemaphoreCreateMutex` |
| NXP AN13036 | FreeRTOS on Cortex-M33 with TrustZone |
| SDK `board.h` | `LED_RED_TOGGLE`, `LOGIC_LED_ON/OFF` macros |
