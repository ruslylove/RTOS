---
theme: seriph
background: https://cover.sli.dev
title: "RTOS · Week 14 — Zephyr RTOS"
info: |
  ## Real-Time Operating Systems
  Week 14 — Zephyr RTOS

  KMUTNB · Faculty of Engineering · Department of Electrical and Computer Engineering
  Master of Engineering (M.Eng.) — Electrical and Computer Engineering
class: text-center
transition: slide-left
mdc: true
routeAlias: week-14
fonts:
  sans: Inter
  serif: Lora
  mono: Fira Code
---

# Real-Time Operating Systems

## Week 14 — Zephyr RTOS

Kconfig · Device Tree · west · Zephyr vs FreeRTOS

<div class="pt-10 opacity-70 text-sm">
  KMUTNB · Faculty of Engineering · M.Eng. in Electrical & Computer Engineering
</div>

<div class="abs-br m-6 text-xs opacity-50">
  Reading: Zephyr docs.zephyrproject.org · Introduction to Zephyr (Nordic DevAcademy)
</div>

---
layout: two-cols
layoutClass: gap-8
---

# From Week 13 to Week 14

We have mastered FreeRTOS: tasks, IPC, memory, power, and security.

But FreeRTOS is not the only real-time OS. The embedded industry is increasingly adopting **Zephyr** — an open-source, Linux Foundation-hosted RTOS with a very different philosophy.

Understanding both systems makes you a better embedded engineer.

::right::

<div class="mt-10 px-5 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-sm leading-relaxed">

**This week — Zephyr RTOS:**

- Zephyr architecture overview
- Kconfig: compile-time configuration
- Device Tree: hardware description language
- west: the Zephyr meta-tool
- Zephyr threading API vs FreeRTOS
- Lab 11: port the producer–consumer from FreeRTOS to Zephyr

</div>

---

# Week 14 — Learning Objectives

By the end of this lecture you will be able to:

<v-clicks>

- **Describe** the Zephyr architecture: kernel, drivers, subsystems, and boards.
- **Use** Kconfig options to enable/disable subsystems at compile time.
- **Read** a Device Tree overlay and explain how it maps hardware to drivers.
- **Use** the `west` tool to build, flash, and debug a Zephyr application.
- **Compare** Zephyr thread API and IPC primitives to their FreeRTOS equivalents.

</v-clicks>

<div v-click class="mt-6 px-4 py-2 border-l-4 border-amber-500 bg-amber-50 dark:bg-amber-900/20 text-sm">
Maps to <b>CLO 5</b> — evaluate different RTOS platforms and their trade-offs for a given application.
</div>

---
layout: section
---

# Part 1
## Zephyr Architecture Overview

---

# What Is Zephyr?

<div class="grid grid-cols-3 gap-4 mt-4 text-sm">

<div class="px-4 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30">
<div class="font-bold">Open source & governed</div>
<div class="mt-2">Linux Foundation project. Apache 2.0 license. Contributions from Intel, Nordic, NXP, ST, Google, Microsoft.</div>
</div>

<div class="px-4 py-4 rounded-lg bg-green-50 dark:bg-green-900/30">
<div class="font-bold">Broad hardware support</div>
<div class="mt-2">600+ boards. ARM, RISC-V, x86, Xtensa, ARC. Single build system for all targets via Device Tree + Kconfig.</div>
</div>

<div class="px-4 py-4 rounded-lg bg-amber-50 dark:bg-amber-900/30">
<div class="font-bold">Designed for IoT</div>
<div class="mt-2">Built-in networking (BLE, Thread, Wi-Fi, LwM2M), TLS/DTLS, FOTA, power management, logging, shell.</div>
</div>

</div>

<div class="mt-5 text-sm">

**Layer stack:**

```
Application
────────────────────────────────────────────────
Zephyr subsystems: net, BT, USB, FS, shell, log
────────────────────────────────────────────────
Zephyr kernel: threads, semaphores, queues, timers
────────────────────────────────────────────────
Drivers (Device Tree-bound: GPIO, I2C, UART, ADC…)
────────────────────────────────────────────────
HAL / SoC support layer (ARM CMSIS, vendor HAL)
────────────────────────────────────────────────
Hardware (MCXN236, nRF5340, STM32, …)
```

</div>

---
layout: section
---

# Part 2
## Kconfig — Compile-Time Configuration

---
layout: two-cols
layoutClass: gap-6
---

# Kconfig Basics

Kconfig is borrowed from the Linux kernel. It controls which code is compiled into the firmware at build time.

```text
# prj.conf — your application configuration
CONFIG_SERIAL=y           # enable UART driver
CONFIG_LOG=y              # enable logging subsystem
CONFIG_LOG_DEFAULT_LEVEL=3

CONFIG_MAIN_STACK_SIZE=2048
CONFIG_HEAP_MEM_POOL_SIZE=4096

CONFIG_BT=n               # disable Bluetooth (save flash)
CONFIG_NETWORKING=n
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_app)

target_sources(app PRIVATE src/main.c)
```

::right::

<div class="mt-6 px-5 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-sm leading-relaxed">

### How Kconfig differs from FreeRTOSConfig.h

| | FreeRTOS | Zephyr |
|---|---------|--------|
| Config file | `FreeRTOSConfig.h` | `prj.conf` (Kconfig) |
| Format | C `#define` | `CONFIG_KEY=value` |
| Dependency checking | None | Kconfig Kconfiglib validates all deps |
| IDE support | Manual | `west build -t menuconfig` GUI |
| Conditional compilation | `#if` in C | Kconfig `depends on` rules |

<div class="mt-3 text-xs opacity-70">
Running `west build -t menuconfig` opens a terminal GUI showing all available options with help text and dependency information.
</div>

</div>

---
layout: section
---

# Part 3
## Device Tree

---

# Device Tree — Hardware as Data

Device Tree (DTS) describes hardware topology in a text format. Drivers are **bound to nodes** at compile time — no runtime probing.

```text
/* boards/arm/frdm_mcxn236/frdm_mcxn236.dts (excerpt) */
&lpuart0 {
    status = "okay";
    current-speed = <115200>;
    pinctrl-0 = <&pinmux_lpuart0>;
};

&lpadc0 {
    status = "okay";
    #address-cells = <1>;
    #size-cells = <0>;
    channel@0 {
        reg = <0>;
        zephyr,gain = "ADC_GAIN_1";
        zephyr,reference = "ADC_REF_INTERNAL";
        zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
        zephyr,resolution = <12>;
    };
};
```

```c
/* Application: access ADC by DT node label */
const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(lpadc0));
```

---

# Device Tree Overlays

For custom hardware (lab boards), add a `.overlay` file instead of modifying the board DTS:

```text
/* app.overlay — add an I2C sensor to the FRDM board */
&lpi2c1 {
    status = "okay";
    clock-frequency = <I2C_BITRATE_FAST>;

    my_sensor: bme280@76 {
        compatible = "bosch,bme280";
        reg = <0x76>;
        status = "okay";
    };
};
```

```c
/* Access the sensor via DT-generated macro */
const struct device *sensor =
    DEVICE_DT_GET(DT_NODELABEL(my_sensor));

sensor_sample_fetch(sensor);
sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &val);
```

<div v-click class="mt-3 text-sm px-4 py-2 border-l-4 border-green-500 bg-green-50 dark:bg-green-900/20">
DT overlays mean your application code never contains I2C addresses or pin numbers. The same C code compiles for a different board by swapping the overlay — the addresses are resolved at build time from DT macros.
</div>

---
layout: section
---

# Part 4
## west — The Zephyr Meta-Tool

---

# west Commands

```bash
# Install west
pip install west

# Initialise a Zephyr workspace
west init -m https://github.com/zephyrproject-rtos/zephyr zephyrproject
cd zephyrproject
west update          # fetch all modules (HALs, crypto, etc.)

# Build for FRDM-MCXN236
cd my_app
west build -b frdm_mcxn236 .

# Flash via J-Link or MCU-Link
west flash

# Open a debug session
west debug

# Interactive Kconfig menu
west build -t menuconfig

# Show full build configuration
west build -t config
```

<div v-click class="mt-3 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
`west` is a wrapper around CMake + ninja. The board name (`-b frdm_mcxn236`) selects the DTS file and SoC HAL automatically. No manual SDK configuration paths needed.
</div>

---
layout: section
---

# Part 5
## Zephyr Threading vs FreeRTOS

---

# API Comparison

<div class="mt-3 text-sm">

| Concept | FreeRTOS | Zephyr |
|---------|---------|--------|
| Task/Thread | `xTaskCreate` | `k_thread_create` or `K_THREAD_DEFINE` |
| Semaphore (binary) | `xSemaphoreCreateBinary` | `k_sem_init` / `K_SEM_DEFINE` |
| Mutex | `xSemaphoreCreateMutex` | `k_mutex_init` / `K_MUTEX_DEFINE` |
| Queue | `xQueueCreate` | `k_msgq_init` / `K_MSGQ_DEFINE` |
| Timer | `xTimerCreate` | `k_timer_init` / `K_TIMER_DEFINE` |
| Delay | `vTaskDelay` | `k_sleep(K_MSEC(n))` |
| Heap alloc | `pvPortMalloc` | `k_malloc` |

</div>

```c
/* Zephyr thread — macro defines stack + thread at file scope */
K_THREAD_DEFINE(sensor_tid, 1024,
                sensor_thread, NULL, NULL, NULL,
                5, 0, 0);   /* priority 5, no flags, start immediately */

void sensor_thread(void *a, void *b, void *c)
{
    struct k_sem ready;
    k_sem_init(&ready, 0, 1);
    for (;;) {
        k_sleep(K_MSEC(500));
        read_and_publish();
    }
}
```

---
layout: section
---

# Part 6
## Lab 11 — Port to Zephyr

---
layout: two-cols
layoutClass: gap-6
---

# Lab 11 — FreeRTOS → Zephyr

<v-clicks>

**Step 1 — Set up workspace:**
1. `west init` + `west update` for Zephyr v3.6
2. Confirm `west build -b frdm_mcxn236 samples/hello_world` compiles

**Step 2 — Port producer–consumer (Lab 9 reuse):**
3. Replace `xQueueCreate` with `K_MSGQ_DEFINE`
4. Replace `xTaskCreate` with `K_THREAD_DEFINE`
5. Replace `xSemaphoreCreateBinary` with `K_SEM_DEFINE`

**Step 3 — Add Device Tree sensor:**
6. Write overlay for BME280 on I2C1
7. Use `sensor_sample_fetch` / `sensor_channel_get` API
8. Log readings with `LOG_INF` (Zephyr logging subsystem)

</v-clicks>

::right::

<div class="mt-8 px-5 py-4 rounded-lg bg-amber-50 dark:bg-amber-900/30 text-sm leading-relaxed">

**What to submit**

- `prj.conf` with Kconfig choices justified
- `app.overlay` for BME280
- Ported producer–consumer source
- Side-by-side API mapping table (your Lab 9 code vs Zephyr equivalents)
- `west build` output showing binary size comparison vs FreeRTOS equivalent

<div class="mt-3 text-xs opacity-70">
Reading — Zephyr docs: Threads, Kernel Services, Sensor API, Logging · Nordic DevAcademy NCS Fundamentals
</div>

</div>

---
layout: default
---

# Key Takeaways

<v-clicks>

- **Zephyr** is a Linux Foundation RTOS designed for IoT: open source, 600+ boards, built-in networking and security subsystems, Apache 2.0 license.
- **Kconfig** replaces `FreeRTOSConfig.h` — text-based, dependency-validated compile-time configuration. `west build -t menuconfig` provides a GUI for discovery.
- **Device Tree** separates hardware description from driver code. Drivers bind to DT nodes at compile time. Overlays let you customise per-board without touching the board BSP.
- **west** is the unified meta-tool: init workspace, update modules, build, flash, debug, all from one command.
- Zephyr and FreeRTOS threading primitives are semantically equivalent — porting is mostly API renaming, but Zephyr's opinionated subsystems (logging, shell, net) add significant value.

</v-clicks>

<div v-click class="mt-5 text-center text-base px-4 py-2 rounded bg-blue-100 dark:bg-blue-900/40">
Module 6 complete. Next — <b>Module 7: Frontiers</b>. Week 15: Mixed-Criticality & Safety Standards.
</div>

---

# Before Next Week

<div class="grid grid-cols-2 gap-8 mt-6">

<div>

### Reading
- **Zephyr documentation** — Getting Started, Threads, Kernel Services
- **Nordic DevAcademy** — nRF Connect SDK Fundamentals (Device Tree module)
- Compare: Zephyr vs FreeRTOS feature matrix (zephyrproject.org)

### Lab
- Complete **Lab 11** — Zephyr port + Device Tree sensor
- Compare binary sizes: FreeRTOS vs Zephyr for the same application

</div>

<div>

### Check yourself
<div class="text-sm">

1. In Zephyr, where does a GPIO pin number come from? (hint: not in the C source)
2. A Kconfig option `CONFIG_BT=y` depends on `CONFIG_ENTROPY_GENERATOR=y`. What happens if you set `CONFIG_BT=y` but forget `CONFIG_ENTROPY_GENERATOR`?
3. You want to add a second I2C device at address 0x48 to the same bus. How do you do it without modifying the board's DTS file?
4. What is the Zephyr equivalent of `pvPortMalloc`?

</div>

</div>

</div>

---
layout: end
class: text-center
---

# Week 14 Complete

Zephyr RTOS

<div class="mt-4 text-sm opacity-70">
Real-Time Operating Systems · KMUTNB · M.Eng. ECE<br/>
Next — Week 15 · Mixed-Criticality &amp; Safety Standards
</div>

<style>
:root { --slidev-theme-primary: #003874; }
.slidev-layout h1 { color: #003874; }
.dark .slidev-layout h1 { color: #7ba7d9; }
table { font-size: 0.92em; }
</style>
