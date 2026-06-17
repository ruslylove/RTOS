# Lab 01 — Toolchain & Environment Verify

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** WeAct STM32H503CBT6 (Cortex-M33, 250 MHz, 128 KB Flash, 32 KB RAM)  
**Estimated time:** 2–3 hours

---

## Objectives

1. Build a FreeRTOS project for STM32H503 using CMake + Ninja.
2. Flash firmware via ST-Link V2 and the STM32CubeIDE VS Code extension.
3. Verify FreeRTOS creates and schedules tasks via UART serial output and the onboard LED.
4. Observe how task priority affects scheduling.
5. Identify and prevent a shared-resource race condition using a mutex.

---

## Prerequisites

Before starting, confirm:

- [ ] **ARM GNU Toolchain 14.2** installed — `arm-none-eabi-gcc` available at  
  `C:/Users/rusle/.mcuxpressotools/arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi/bin`  
  (add to PATH or set `CMAKE_C_COMPILER` in CMake)
- [ ] **STM32CubeH5 firmware** extracted at  
  `C:/Users/rusle/STM32Cube/Repository/STM32Cube_FW_H5_V1.6.0`  
  (default for STM32CubeIDE — override via `-DSTM32_CUBE_FW=<path>` in CMake)
- [ ] **FreeRTOS-Kernel** cloned to `C:/FreeRTOS-Kernel`:
  ```
  git clone https://github.com/FreeRTOS/FreeRTOS-Kernel C:/FreeRTOS-Kernel
  ```
  Override via `-DFREERTOS_KERNEL_PATH=<path>` if you cloned elsewhere.
- [ ] **Ninja** build system on PATH (`choco install ninja` or download from github.com/ninja-build/ninja)
- [ ] **STM32CubeIDE VS Code Extension** installed (provides `stlinkgdbtarget` debugger)
- [ ] WeAct STM32H503CBT6 board connected via **ST-Link V2** (or onboard ST-Link if present)
- [ ] **USB-UART adapter** connected to PA9 (TX) and GND — opens serial at 115200 baud

### Hardware pin summary

| Signal  | MCU pin | Notes |
|---------|---------|-------|
| LED     | PC13    | Active-low — GPIO LOW = LED ON |
| UART TX | PA9     | USART1 TX (AF7) — connect to USB-UART RX |
| UART RX | PA10    | USART1 RX (AF7) — optional for this lab |

---

## Project Structure

```
lab01_setup_verify/
├── CMakeLists.txt            ← build definition
├── CMakePresets.json         ← Debug / Release presets
├── Inc/
│   ├── main.h                ← LED / UART pin macros
│   └── FreeRTOSConfig.h      ← kernel configuration
├── Src/
│   └── main.c                ← tasks, clock init, UART init
└── .vscode/
    └── launch.json           ← ST-Link debug configuration
```

The build automatically pulls shared sources from:
- `stm32_labs/shared/` — `syscall.c`, `sysmem.c`, linker script
- `STM32_CUBE_FW/Drivers/` — HAL, CMSIS startup, system init
- `FREERTOS_KERNEL_PATH/` — FreeRTOS kernel + Cortex-M33 NTZ port

### What the code does

| Task | Priority | Period | Action |
|------|----------|--------|--------|
| `vBlinkTask` | 2 (HIGH) | 500 ms | Toggles LED on PC13, prints `LED ON/OFF` |
| `vHeartbeatTask` | 1 (LOW) | 1000 ms | Prints FreeRTOS tick uptime in ms |

A shared **UART mutex** (`xUartMutex`) ensures both tasks' `printf` calls don't interleave.  
`vApplicationTickHook` calls `HAL_IncTick()` so HAL timeout functions remain accurate under FreeRTOS.

---

## Part A — Build and Flash

### Step 1 — Configure

```bash
cd stm32_labs/lab01_setup_verify
cmake --preset Debug
```

Expected: Ninja build files generated in `build/Debug/`.

> **Toolchain not found?** Add `arm-none-eabi-gcc` to PATH, or pass  
> `-DCMAKE_C_COMPILER=C:/path/to/arm-none-eabi-gcc` to the cmake command.

> **FreeRTOS not found?** Clone FreeRTOS-Kernel to `C:/FreeRTOS-Kernel` (see Prerequisites).

### Step 2 — Build

```bash
cmake --build build/Debug
```

Expected output (sizes approximate):
```
   text    data     bss     dec filename
  28000     200   22000   50200  lab01-stm32.elf
```

### Step 3 — Flash

Open VS Code in `lab01_setup_verify/`, press **F5** (or Run → Start Debugging).  
The ST-Link extension erases, programs, and resets the MCU automatically.

Alternatively, use STM32CubeProgrammer CLI:
```bash
STM32_Programmer_CLI -c port=SWD -w build/Debug/lab01-stm32.bin 0x08000000 -rst
```

---

## Part B — Verify: Serial Output and LED

### Serial terminal

Connect a USB-UART adapter: adapter RX → PA9, GND → GND.  
Open a terminal at **115200 baud, 8N1** (e.g., PuTTY, Tera Term, or `minicom`).

### Expected output

```
=== RTOS Lab 01: Toolchain & Environment Verify ===
Platform: WeAct STM32H503CBT6  |  CPU: 250 MHz Cortex-M33
Tasks: Blink (500 ms) | Heartbeat (1000 ms)

[MAIN] Starting scheduler...

[BLINK] LED ON   (toggle #1)
[BLINK] LED OFF  (toggle #2)
[HB]    uptime = 1001 ms
[BLINK] LED ON   (toggle #3)
[BLINK] LED OFF  (toggle #4)
[HB]    uptime = 2003 ms
...
```

**Checkpoint A:** Capture ≥ 5 seconds of serial output and observe the LED blinking.

---

## Part C — Experiments

### Experiment 1 — Priority swap

Swap the task priorities in `main.c`:

```c
xTaskCreate(vBlinkTask,     "Blink", 256, NULL, 1, NULL); /* was 2 */
xTaskCreate(vHeartbeatTask, "HB",    256, NULL, 2, NULL); /* was 1 */
```

> **Question 1:** Does the interleaving of output lines change? Why or why not?  
> *(Hint: both tasks spend most of their time blocked. Think about what happens  
> when two tasks unblock at the same tick.)*

---

### Experiment 2 — Equal priorities

Set both to priority 1. Rebuild and run.

> **Question 2:** What scheduling policy does FreeRTOS apply to equal-priority ready tasks?  
> Which `FreeRTOSConfig.h` macro controls this?

---

### Experiment 3 — Short blink period

Restore original priorities. Change `vBlinkTask`'s delay to **50 ms**:
```c
vTaskDelay(pdMS_TO_TICKS(50));
```

> **Question 3:** Using $U = C/T$, estimate `vBlinkTask`'s CPU utilization  
> ($C$ = time to execute body, very short; $T$ = 50 ms).  
> Does the heartbeat still appear on time? Why?

---

### Experiment 4 — Remove the mutex (observe the race)

Comment out the `xSemaphoreTake` / `xSemaphoreGive` pairs around `printf` in **both tasks**.  
Keep the 50 ms blink period to maximize interleaving.

> **Question 4:** Do you see garbled output? Why is this a race condition on a  
> single-core MCU? What property of a mutex prevents it?

Restore the mutex before submitting.

---

## Deliverables

| # | Item |
|---|------|
| 1 | Serial capture (≥ 5 s) showing correct `[BLINK]` / `[HB]` output |
| 2 | Photo or video of PC13 LED blinking |
| 3 | Written answers to Questions 1–4 |
| 4 | Modified `main.c` from Experiment 3 (50 ms Blink, mutex restored) |

---

## Key Concepts

| Concept | Where you saw it |
|---------|-----------------|
| Preemptive priority scheduling | Priority swap (Exp 1) |
| Round-robin time-slicing | Equal priorities (Exp 2) |
| CPU utilization $U = C/T$ | 50 ms period (Exp 3) |
| Race condition on shared resource | Mutex removal (Exp 4) |
| Mutex as critical-section guard | `xUartMutex` throughout |

---

## Implementation Notes

### Clock configuration

The lab uses HSI (64 MHz internal oscillator) fed through PLL1:

| Parameter | Value |
|-----------|-------|
| HSI       | 64 MHz |
| PLLM      | 16 → VCI = 4 MHz |
| PLLN      | 125 → VCO = 500 MHz |
| PLLP      | 2 → SYSCLK = 250 MHz |
| Flash WS  | 5 (VOS0 at 250 MHz) |

If your board has a 24 MHz HSE crystal, you can switch to HSE/12 × 250/2 = 250 MHz  
by changing `RCC_OSCILLATORTYPE_HSI` to `RCC_OSCILLATORTYPE_HSE` and adjusting PLLM.

### FreeRTOS port choice

This lab uses `GCC/ARM_CM33_NTZ/non_secure` — the Cortex-M33 port **without** TrustZone  
secure-world separation. The STM32H503 supports TrustZone but this lab set runs entirely  
in non-secure (or secure-only) mode with `CMSIS_Dtz = NO_TZ`.

### vApplicationTickHook and HAL

FreeRTOS owns `SysTick_Handler` once the scheduler starts. The HAL tick counter  
(`uwTick`) stops incrementing unless we call `HAL_IncTick()` from within the  
FreeRTOS tick hook. This is why `configUSE_TICK_HOOK 1` and the hook implementation  
are mandatory in all these labs.
