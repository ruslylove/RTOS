# STM32 FreeRTOS Lab Set
### WeAct STM32H503CBT6 · Cortex-M33 · 250 MHz · 128 KB Flash · 32 KB RAM

Ten hands-on labs covering embedded RTOS fundamentals, mirroring the NXP FRDM-MCXN236 lab set.

---

## Prerequisites

### 1. Clone FreeRTOS-Kernel

```powershell
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git C:/FreeRTOS-Kernel
```

The default branch (`main`) is used. All labs reference `C:/FreeRTOS-Kernel`.

### 2. STM32CubeH5 SDK

Already installed at:
```
C:/Users/rusle/STM32Cube/Repository/STM32Cube_FW_H5_V1.6.0
```

### 3. ARM toolchain

Located at:
```
C:/Users/rusle/.mcuxpressotools/arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi/bin
```

### 4. Build tools

- **CMake ≥ 3.20** and **Ninja** — available in PATH
- **STM32CubeProgrammer 2.22.0** — for flashing via ST-Link V2

---

## Board Pinout

| Signal   | Pin  | AF    | Notes                   |
|----------|------|-------|-------------------------|
| LED      | PC13 | GPIO  | Active-low (LOW = ON)   |
| UART TX  | PA9  | AF7   | USART1, 115200 8N1      |
| UART RX  | PA10 | AF7   | USART1                  |
| SWD CLK  | PA14 | —     | ST-Link V2              |
| SWD DIO  | PA13 | —     | ST-Link V2              |

---

## Build & Flash (any lab)

```powershell
cd stm32_labs\lab01_setup_verify

# Configure (first time or after CMakeLists changes)
cmake --preset Debug

# Build
cmake --build build/Debug

# Flash via STM32CubeProgrammer CLI
"C:/Users/rusle/AppData/Local/stm32cube/bundles/programmer/2.22.0+st.1/bin/STM32_Programmer_CLI.exe" `
    -c port=SWD -w build/Debug/lab01-stm32.hex -v -rst
```

Or open the lab folder in VS Code and press **F5** to build + flash + debug via the `stlinkgdbtarget` extension.

---

## Lab Overview

| Lab | Topic | Key FreeRTOS APIs |
|-----|-------|-------------------|
| [Lab 01](lab01_setup_verify/) | Environment Verify — LED blink + UART printf | `xTaskCreate`, `vTaskDelay`, mutex |
| [Lab 02](lab02_freertos_basics/) | FreeRTOS Basics — queues, semaphores, timers | `xQueueSend`, `xSemaphoreGive`, `xTimerCreate` |
| [Lab 03](lab03_aperiodic_server/) | Aperiodic Servers & Pipeline | semaphore server, polling server, pipeline queue |
| [Lab 04](lab04_priority_inversion/) | Priority Inversion & Inheritance | binary semaphore vs mutex, busy-work spin |
| [Lab 05](lab05_watchdog/) | Hardware Watchdog (IWDG) | per-task SW watchdog table, LSI timer |
| [Lab 06](lab06_wcet_dwt/) | WCET Measurement (DWT) | `DWT->CYCCNT`, FPU latency, FIR, sort |
| [Lab 07](lab07_stack_overflow/) | Stack Overflow Detection | `configCHECK_FOR_STACK_OVERFLOW`, watermarks |
| [Lab 08](lab08_mpu_protection/) | MPU Memory Protection | ARM v8-M MPU, RBAR/RLAR, MemManage fault |
| [Lab 09](lab09_advanced_ipc/) | Advanced IPC | Event Groups, Task Notifications, Stream Buffers |
| [Lab 10](lab10_tickless_idle/) | Tickless Idle / Low Power | `configUSE_TICKLESS_IDLE`, WFI, tick suppression |

---

## Shared Infrastructure

All labs share code under `shared/` and `cmake/`:

```
stm32_labs/
├── cmake/
│   ├── gnu-tools-for-stm32.cmake   # toolchain: arm-none-eabi-gcc
│   └── stm32_lab_common.cmake      # HAL sources, FreeRTOS sources, include dirs
├── shared/
│   ├── Inc/stm32h5xx_hal_conf.h    # HAL module enable switches
│   ├── Src/syscall.c               # Newlib stubs + __io_putchar hook
│   ├── Src/sysmem.c                # _sbrk heap allocator
│   └── stm32h503xb_flash.ld        # Linker script: 128K flash, 32K RAM
└── lab0X_<name>/
    ├── CMakeLists.txt              # project("labXX-stm32") + LAB_SOURCES
    ├── CMakePresets.json           # Debug/Release presets, CMSIS device vars
    ├── Inc/FreeRTOSConfig.h        # Per-lab FreeRTOS configuration
    ├── Inc/<lab>.h
    ├── Src/main.c                  # HAL init, clock, UART, tasks, hooks
    ├── Src/<lab>.c
    ├── .vscode/launch.json         # ST-Link GDB debug config
    └── lab0X_instructions.md
```

---

## Clock Configuration

All labs run at **250 MHz** using HSI PLL:

```
HSI (64 MHz) → PLLM=16 → 4 MHz VCO input
             → PLLN=125 → 500 MHz VCO
             → PLLP=2  → 250 MHz SYSCLK
Flash latency: WS=5, VOS0
```

---

## FreeRTOS Configuration (defaults across all labs)

| Setting | Value | Notes |
|---------|-------|-------|
| `configCPU_CLOCK_HZ` | 250 000 000 | |
| `configTICK_RATE_HZ` | 1000 | 1 ms tick |
| `configTOTAL_HEAP_SIZE` | 20 KB | heap_4 |
| `configCHECK_FOR_STACK_OVERFLOW` | 2 | watermark method |
| `configUSE_TICK_HOOK` | 1 | calls `HAL_IncTick()` |
| `configPRIO_BITS` | 4 | 16 priority levels |
| `configKERNEL_INTERRUPT_PRIORITY` | 0xF0 (level 15) | |
| `configMAX_SYSCALL_INTERRUPT_PRIORITY` | 0x50 (level 5) | |
| FreeRTOS port | `GCC/ARM_CM33_NTZ/non_secure` | Cortex-M33, no TrustZone |

---

## Troubleshooting

**Build fails: FreeRTOS headers not found**
```
Clone FreeRTOS-Kernel to exactly C:/FreeRTOS-Kernel
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git C:/FreeRTOS-Kernel
```

**Flash fails: ST-Link not found**
```
Check USB connection and run:
STM32_Programmer_CLI.exe -l
```

**printf produces no output**
Verify USART1 PA9/PA10 wiring and that terminal is set to 115200 baud, 8N1, no flow control.

**HardFault on startup**
Likely a linker script or stack size issue. Attach GDB and read `CFSR` (0xE000ED28) and `HFSR` (0xE000ED2C) to identify the fault type.
