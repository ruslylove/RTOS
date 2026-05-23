# Lab 11 — Zephyr RTOS: Port, Device Tree & Kconfig

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** NXP FRDM-MCXN236 (Cortex-M33)  
**Estimated time:** 4–5 hours  
**Builds on:** Lab 03 (producer-consumer pipeline), Lab 07 (memory analysis)

---

## Objectives

By completing this lab you will be able to:

1. **Set up** a Zephyr RTOS workspace for the FRDM-MCXN236 using `west`.
2. **Port** the Lab 03 producer-consumer pipeline from FreeRTOS to Zephyr threading APIs.
3. **Write** a Device Tree overlay to add a simulated BME280 sensor on I2C.
4. **Use** the Zephyr Sensor API (`sensor_sample_fetch` / `sensor_channel_get`) with a DT-bound driver.
5. **Compare** binary size, API surface, and configuration overhead between FreeRTOS and Zephyr.

---

## Prerequisites

- [ ] Lab 03 producer-consumer source (for reference during porting).
- [ ] Python 3.8+ and `pip` available.
- [ ] At least 4 GB free disk space (Zephyr workspace + HALs).
- [ ] Git installed.
- [ ] USB connection to FRDM-MCXN236 (J-Link or MCU-Link).

> **Time note:** `west update` downloads ~1.5 GB of source. Do this on a fast network connection. The download is a one-time setup; subsequent builds are fast.

---

## Background

### Zephyr vs FreeRTOS — key differences

| Aspect | FreeRTOS | Zephyr |
|--------|---------|--------|
| Configuration | `FreeRTOSConfig.h` (`#define`) | `prj.conf` (Kconfig) |
| Hardware description | Code (`GPIO_PinInit(...)`) | Device Tree (`.dts` / `.overlay`) |
| Build system | Make / CMake (manual) | `west` (unified) |
| Driver model | SDK-specific (`fsl_gpio.h`) | Unified API (`gpio.h`) |
| Networking / BT | External libraries | Built-in subsystems |
| Certification | Qualified kits available | Partial (Safety Cert. project) |

---

## Project Structure

```
lab11_zephyr/
├── CMakeLists.txt
├── prj.conf                    ← Kconfig (replaces FreeRTOSConfig.h)
├── app.overlay                 ← Device Tree customisation
├── src/
│   ├── main.c                  ← Zephyr entry, thread definitions
│   ├── sensor_thread.c/.h      ← producer (replaces vSensor1Task)
│   ├── logger_thread.c/.h      ← consumer (replaces vLoggerTask)
│   └── monitor_thread.c/.h     ← event group analogue
└── boards/
    └── frdm_mcxn236.conf       ← board-specific overrides (optional)
```

---

## Part A — Workspace Setup

### Step 1 — Install west

```bash
pip install west
```

### Step 2 — Initialise Zephyr workspace

```bash
mkdir ~/zephyrproject && cd ~/zephyrproject
west init -m https://github.com/zephyrproject-rtos/zephyr --mr v3.6.0
west update          # ~20 minutes on first run
west zephyr-export   # registers CMake package
pip install -r zephyr/scripts/requirements.txt
```

### Step 3 — Verify with hello_world

```bash
cd ~/zephyrproject
west build -b frdm_mcxn236 zephyr/samples/hello_world
west flash
```

Expected terminal output:
```
*** Booting Zephyr OS build v3.6.0 ***
Hello World! frdm_mcxn236
```

**Checkpoint A:** Screenshot of hello_world running on the board.

> **Question A1:** `west update` fetches many modules (HALs, mbedTLS, LVGL, etc.). Which module provides the NXP MCX HAL for the FRDM-MCXN236? (Hint: look in `zephyrproject/modules/hal/`.)

---

## Part B — Port the Producer-Consumer Pipeline

### Step 1 — FreeRTOS → Zephyr API mapping

Before writing any code, complete this mapping table (fill in the Zephyr column):

| FreeRTOS (Lab 03) | Zephyr equivalent |
|-------------------|------------------|
| `xTaskCreate(fn, name, stack_words, ...)` | |
| `K_THREAD_DEFINE(tid, stack_bytes, fn, ...)` | |
| `xQueueCreate(depth, item_size)` | |
| `xQueueSend(q, &item, timeout)` | |
| `xQueueReceive(q, &item, portMAX_DELAY)` | |
| `xSemaphoreCreateBinary()` | |
| `xSemaphoreGive(sem)` | |
| `xSemaphoreTake(sem, portMAX_DELAY)` | |
| `vTaskDelay(pdMS_TO_TICKS(n))` | |
| `vTaskDelayUntil(&last, period)` | |

### Step 2 — `prj.conf`

```kconfig
# prj.conf
CONFIG_SERIAL=y
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3

# Threading
CONFIG_MAIN_STACK_SIZE=2048
CONFIG_HEAP_MEM_POOL_SIZE=4096

# Message queue (replaces FreeRTOS queue)
CONFIG_CBPRINTF_FP_SUPPORT=n   # save flash — no float printf
```

### Step 3 — `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lab11_zephyr)

target_sources(app PRIVATE
    src/main.c
    src/sensor_thread.c
    src/logger_thread.c
    src/monitor_thread.c
)
```

### Step 4 — Sensor producer thread

```c
/* src/sensor_thread.c */
#include <zephyr/kernel.h>
#include "sensor_thread.h"

/* Message queue: 10 items of SensorReading_t */
K_MSGQ_DEFINE(sensor_msgq, sizeof(SensorReading_t), 10, 4);

static void sensor_thread_fn(void *a, void *b, void *c)
{
    SensorReading_t reading = { .sensor_id = 1, .value = 0 };

    while (1) {
        k_sleep(K_MSEC(100));
        reading.value   = (reading.value + 7) % 1000;
        reading.ts_ms   = k_uptime_get_32();

        if (k_msgq_put(&sensor_msgq, &reading, K_NO_WAIT) != 0)
            LOG_WRN("queue full — dropped");
    }
}

K_THREAD_DEFINE(sensor_tid, 1024, sensor_thread_fn,
                NULL, NULL, NULL, 5, 0, 0);
```

### Step 5 — Logger consumer thread

```c
/* src/logger_thread.c */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "sensor_thread.h"   /* for sensor_msgq extern */

LOG_MODULE_REGISTER(logger, LOG_LEVEL_INF);

static void logger_thread_fn(void *a, void *b, void *c)
{
    SensorReading_t r;
    uint32_t count = 0;
    while (1) {
        k_msgq_get(&sensor_msgq, &r, K_FOREVER);
        count++;
        LOG_INF("#%u  sensor=%d  val=%d  t=%u ms",
                count, r.sensor_id, r.value, r.ts_ms);
    }
}

K_THREAD_DEFINE(logger_tid, 1024, logger_thread_fn,
                NULL, NULL, NULL, 3, 0, 0);
```

### Step 6 — Build and flash

```bash
cd ~/zephyrproject/lab11_zephyr
west build -b frdm_mcxn236 .
west flash
```

Expected output via UART:
```
[00:00:00.012,000] <inf> logger: #1  sensor=1  val=7  t=100 ms
[00:00:00.112,000] <inf> logger: #2  sensor=1  val=14  t=200 ms
...
```

**Checkpoint B:** Terminal showing Zephyr logging output from the ported pipeline.

> **Question B1:** In FreeRTOS, `xQueueSend` with `portMAX_DELAY` blocks the calling task forever. What is the Zephyr equivalent timeout value for "wait forever"?

> **Question B2:** `K_THREAD_DEFINE` takes stack size in **bytes** while FreeRTOS `xTaskCreate` takes it in **words** (4 bytes each). Convert the Lab 03 sensor task's 128-word stack to the correct Zephyr byte value.

---

## Part C — Device Tree Sensor (BME280)

### Step 1 — Write the overlay

Create `app.overlay` in the project root:

```text
/* app.overlay — add BME280 sensor on LPI2C1 */
&lpi2c1 {
    status = "okay";
    clock-frequency = <I2C_BITRATE_STANDARD>;
    pinctrl-0 = <&pinmux_lpi2c1>;
    pinctrl-names = "default";

    bme280: bme280@76 {
        compatible = "bosch,bme280";
        reg = <0x76>;
        status = "okay";
    };
};
```

Add to `prj.conf`:
```kconfig
CONFIG_I2C=y
CONFIG_SENSOR=y
CONFIG_BME280=y
```

### Step 2 — Use the Sensor API

Add a sensor reading thread in `src/sensor_thread.c`:

```c
#include <zephyr/drivers/sensor.h>

static const struct device *bme280 = DEVICE_DT_GET(DT_NODELABEL(bme280));

static void bme_thread_fn(void *a, void *b, void *c)
{
    if (!device_is_ready(bme280)) {
        LOG_ERR("BME280 not ready");
        return;
    }

    struct sensor_value temp, pressure, humidity;
    while (1) {
        k_sleep(K_MSEC(1000));
        sensor_sample_fetch(bme280);
        sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(bme280, SENSOR_CHAN_PRESS, &pressure);
        sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY, &humidity);
        LOG_INF("T=%.2f°C  P=%.2f hPa  H=%.2f %%",
                sensor_value_to_double(&temp),
                sensor_value_to_double(&pressure) / 1000.0,
                sensor_value_to_double(&humidity));
    }
}

K_THREAD_DEFINE(bme_tid, 1024, bme_thread_fn, NULL, NULL, NULL, 4, 0, 0);
```

> **Note:** If you do not have a physical BME280, use the Zephyr emulator:
> ```kconfig
> CONFIG_BME280_TRIGGER_NONE=y
> CONFIG_EMUL=y
> CONFIG_BME280_EMUL=y
> ```
> The emulator returns synthetic values, letting you exercise the full API without hardware.

**Checkpoint C:** Terminal showing BME280 (or emulated) temperature/pressure/humidity readings every second.

> **Question C1:** Where in the Zephyr source tree is the BME280 driver located? What is the value of its `compatible` string in the DTS binding file?

> **Question C2:** You want to add a second BME280 at I2C address 0x77 on the same bus. What change do you make — in `app.overlay` only, or also in C code? Why?

---

## Part D — Binary Size Comparison

### Step 1 — Measure Zephyr binary size

```bash
west build -b frdm_mcxn236 .
arm-none-eabi-size build/zephyr/zephyr.elf
```

### Step 2 — Measure equivalent FreeRTOS binary size

From Lab 03:
```bash
cd ../lab03_aperiodic_server
make
arm-none-eabi-size lab03.elf
```

### Step 3 — Complete the comparison table

| Metric | FreeRTOS (Lab 03) | Zephyr (Lab 11) |
|--------|------------------|----------------|
| `.text` (code) | | |
| `.rodata` (const) | | |
| `.data` + `.bss` (RAM) | | |
| Total flash | | |
| Build time (cold) | | |
| Lines of config | | |

> **Question D1:** Zephyr is typically larger than FreeRTOS for the same feature set. Where does most of the extra flash usage come from? (Hint: run `west build -t rom_report`.)

> **Question D2:** You need to add BLE advertising to this application. Describe how you would do this in Zephyr (which `prj.conf` options, which API) vs how you would approach it with FreeRTOS (which external library, which integration steps). Which is faster to integrate and why?

---

## Part E — Kconfig Exploration (Optional Challenge)

### Explore available options

```bash
west build -b frdm_mcxn236 -t menuconfig .
```

This opens an interactive terminal menu. Navigate to:
- `Kernel → Threads` — find `CONFIG_MAIN_STACK_SIZE`
- `Subsystems → Logging` — observe log level options and backend choices
- `Drivers → Sensor` — note available sensor drivers

> **Challenge question:** Disable the shell (`CONFIG_SHELL=n`) and the logging backend to serial (`CONFIG_LOG_BACKEND_UART=n`). Rebuild and measure the flash savings. How much code do logging and the shell add to a Zephyr application?

---

## Deliverables

| # | Item |
|---|------|
| 1 | Part A screenshot — hello_world running |
| 2 | Part B completed FreeRTOS→Zephyr API mapping table |
| 3 | Part B terminal — Zephyr producer-consumer output |
| 4 | Part C terminal — BME280 (real or emulated) sensor readings |
| 5 | Part D binary size comparison table |
| 6 | `prj.conf` and `app.overlay` file listings |
| 7 | Written answers to Questions A1, B1–B2, C1–C2, D1–D2 |
| 8 | (Optional) Part E challenge — flash savings measurement |

---

## Key Concepts Demonstrated

| Concept | Where you saw it |
|---------|-----------------|
| west workspace init and build | Part A |
| Kconfig compile-time configuration | Part B `prj.conf` |
| `K_THREAD_DEFINE` and `K_MSGQ_DEFINE` | Part B porting |
| Device Tree overlay for I2C sensor | Part C |
| Unified Sensor API | Part C |
| FreeRTOS vs Zephyr binary size | Part D |

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| Zephyr documentation | Getting Started, Threads, Message Queue, Sensor API |
| Zephyr Device Tree | `boards/arm/frdm_mcxn236/`, `dts/bindings/sensor/bosch,bme280.yaml` |
| Nordic DevAcademy | NCS Fundamentals — Device Tree module |
| FreeRTOS docs | `xQueueCreate` (for comparison) |
