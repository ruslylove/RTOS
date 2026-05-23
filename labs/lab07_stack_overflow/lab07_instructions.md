# Lab 07 — Stack Overflow Detection & Memory Analysis

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** NXP FRDM-MCXN236 (Cortex-M33)  
**Estimated time:** 2–3 hours  
**Builds on:** Lab 03 (multi-task system), Lab 06 (DWT, task parameters)

---

## Objectives

By completing this lab you will be able to:

1. **Induce** a stack overflow in a FreeRTOS task using a recursive function.
2. **Detect** the overflow using `configCHECK_FOR_STACK_OVERFLOW = 2` before corruption spreads.
3. **Right-size** task stacks using `uxTaskGetStackHighWaterMark`.
4. **Analyse** total heap usage with `xPortGetFreeHeapSize` and the linker map file.
5. **Convert** one task to static allocation and verify heap usage drops.

---

## Prerequisites

- [ ] Lab 03 producer-consumer system (used as the baseline in Part C).
- [ ] ARM GNU Toolchain and `pyocd` working.
- [ ] A text editor with hex search (for observing the 0xA5 pattern).

---

## Background

### Stack overflow consequences

Every FreeRTOS task has its own stack allocated from the heap. If a task's stack grows beyond its allocation, it silently overwrites the adjacent heap block — which may contain another task's TCB, stack, or queue buffer. The crash typically manifests as a hard fault far from the actual cause, making debugging very difficult.

FreeRTOS provides two overflow detection mechanisms:

| Method | `configCHECK_FOR_STACK_OVERFLOW` | How it works | Catches |
|--------|----------------------------------|-------------|---------|
| Method 1 | `1` | Check SP < stack base at each context switch | Overflows that leave SP below base |
| Method 2 | `2` | Fill entire stack with 0xA5 at creation; check last 20 bytes at each switch | Most overflows, including those that recover before the switch |

---

## Project Structure

```
lab07_stack_overflow/
├── Makefile
├── linker_MCXN236.ld
├── src/
│   ├── main.c                 ← task creation
│   ├── overflow_demo.c/.h     ← Part A: intentional overflow
│   ├── stack_sizing.c/.h      ← Part B: sizing the Lab 03 pipeline
│   ├── static_alloc.c/.h      ← Part C: static allocation
│   ├── FreeRTOSConfig.h
│   └── uart.h / uart.c
└── FreeRTOS-Kernel/
```

---

## Part A — Induce and Detect a Stack Overflow

### Step 1 — Create a task with a small stack

```c
/* overflow_demo.c */

/* Recursive function: each call adds ~64 bytes to the stack */
static uint32_t recursive_sum(uint32_t n)
{
    uint32_t local_buffer[8];   /* 32 bytes local data to grow the stack faster */
    for (int i = 0; i < 8; i++) local_buffer[i] = i + n;
    if (n == 0) return 0;
    return local_buffer[0] + recursive_sum(n - 1);
}

void vOverflowTask(void *pv)
{
    for (;;) {
        uart_printf("[OVF] starting recursive_sum(60)...\r\n");
        uint32_t result = recursive_sum(60);
        uart_printf("[OVF] result = %lu (you should never see this)\r\n", result);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

In `main.c`, create the task with a very small stack (64 words = 256 bytes):

```c
xTaskCreate(vOverflowTask, "Overflow", 64, NULL, 2, NULL);
```

### Step 2 — First run: no detection

Build and flash with detection **disabled**:

```c
/* FreeRTOSConfig.h */
#define configCHECK_FOR_STACK_OVERFLOW  0
```

Observe the terminal. The overflow will likely cause a **HardFault** or garbled output, but no useful diagnostic message.

**Checkpoint A1:** UART capture showing the crash — note that the crash is non-deterministic and may look different each run.

### Step 3 — Enable overflow detection

```c
/* FreeRTOSConfig.h */
#define configCHECK_FOR_STACK_OVERFLOW  2
```

Implement the hook in `overflow_demo.c`:

```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    taskDISABLE_INTERRUPTS();
    uart_printf("\r\n!!! STACK OVERFLOW in task: %s !!!\r\n", pcTaskName);
    /* In production: log to non-volatile storage, then reset */
    for (;;) {}   /* halt here so we can see the message */
}
```

Rebuild and flash.

**Checkpoint A2:** UART capture showing `!!! STACK OVERFLOW in task: Overflow !!!`.

> **Question A1:** With `configCHECK_FOR_STACK_OVERFLOW = 2`, the check only fires at context switch time — not at the exact moment of overflow. Is there any scenario where the overflow happens, the stack "heals" before the switch, and the hook does NOT fire? When would you use Method 1 vs Method 2?

> **Question A2:** The overflow hook receives `pcTaskName`. Can you always trust this name to identify the crashed task? (Hint: what was overwritten just before the hook was called?)

---

## Part B — Right-Sizing Stack Depths

### Step 1 — Find the high-water mark

Increase the overflow task's stack to 512 words so it can run safely:

```c
xTaskCreate(vOverflowTask, "Overflow", 512, NULL, 2, NULL);
```

After the system has run for at least 30 seconds (let the task run 15 times), read the high-water mark:

```c
void vStackReportTask(void *pv)  /* runs once after 30 s */
{
    vTaskDelay(pdMS_TO_TICKS(30000));
    uart_printf("\r\n=== Stack High-Water Mark Report ===\r\n");
    /* Print for each task handle you have */
    TaskHandle_t handles[] = { xOverflowHandle, xSensor1Handle, xLoggerHandle, ... };
    const char  *names[]   = { "Overflow", "Sensor1", "Logger", ... };
    for (int i = 0; i < sizeof(handles)/sizeof(handles[0]); i++) {
        UBaseType_t hwm = uxTaskGetStackHighWaterMark(handles[i]);
        uart_printf("  %-16s  HWM = %u words remaining\r\n", names[i], hwm);
    }
    uart_printf("Free heap: %u bytes\r\n", xPortGetFreeHeapSize());
    vTaskDelete(NULL);
}
```

### Step 2 — Compute the right-sized stack

For each task, apply the formula:

```
right_sized_stack = (allocated_words - HWM) + ceil((allocated_words - HWM) * 0.20)
```

That is: minimum observed usage + 20% headroom.

### Step 3 — Apply to the Lab 03 pipeline

Import the task handles from Lab 03 and build the complete high-water-mark table:

| Task | Allocated (words) | HWM remaining | Min used | Right-sized (+20%) |
|------|------------------|--------------|---------|-------------------|
| vSensor1Task | | | | |
| vSensor2Task | | | | |
| vLoggerTask | | | | |
| vMonitorTask | | | | |
| vOverflowTask | | | | |

**Checkpoint B:** Filled-in table from UART output.

> **Question B1:** `uxTaskGetStackHighWaterMark` returns 4 for one of your tasks. What does this mean? Is this task safe to leave running? What is your next step?

> **Question B2:** The formula adds 20% headroom. What real-world factors could cause a task to use more stack in production than was observed in the lab? Name at least three.

---

## Part C — Heap Analysis

### Step 1 — Total heap usage

FreeRTOS allocates task stacks and TCBs from the heap. Measure usage before and after creating all tasks:

```c
/* In main.c, before vTaskStartScheduler: */
uart_printf("Heap free before scheduler: %u bytes\r\n", xPortGetFreeHeapSize());
uart_printf("Heap minimum ever free:     %u bytes\r\n", xPortGetMinimumEverFreeHeapSize());
```

### Step 2 — Linker map analysis

Build with map output enabled (already configured in `Makefile`):

```bash
make MAP=1
```

Open `lab07.map` and find:

```
Memory Configuration

Name            Origin     Length     Attributes
flash           0x00000000 0x00200000 xr
sram            0x20000000 0x00020000 xrw

Linker script and memory map

.text           0x00000000  <size>   # code + const data (Flash)
.data           0x20000000  <size>   # initialised globals (SRAM, copied from Flash)
.bss            0x20001234  <size>   # zero-init globals (SRAM)
._user_heap     0x200xxxxx  <size>   # FreeRTOS heap (ucHeap array)
```

Fill in this table:

| Section | Size (bytes) | Location | Contains |
|---------|-------------|----------|---------|
| `.text` | | Flash | Code, string literals, ISR vectors |
| `.rodata` | | Flash | `const` data |
| `.data` | | SRAM | Initialised globals |
| `.bss` | | SRAM | Zero-init globals (including `ucHeap`) |
| Stack (MSP) | | SRAM | Main stack (before scheduler) |

**Checkpoint C:** Filled-in table from the linker map.

---

## Part D — Static Allocation

### Step 1 — Convert one task to static allocation

Convert `vLoggerTask` from dynamic to static allocation:

```c
/* static_alloc.c */
static StaticTask_t  xLoggerTaskBuffer;
static StackType_t   xLoggerStack[256];   /* right-sized from Part B */

TaskHandle_t xLoggerHandle = xTaskCreateStatic(
    vLoggerTask,
    "Logger",
    256,
    NULL,
    1,
    xLoggerStack,
    &xLoggerTaskBuffer
);
```

Ensure `FreeRTOSConfig.h` has:
```c
#define configSUPPORT_STATIC_ALLOCATION   1
#define configSUPPORT_DYNAMIC_ALLOCATION  1

/* Required idle task static buffers */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize)
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t  xIdleTaskStack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = xIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}
```

### Step 2 — Measure heap change

Run `xPortGetFreeHeapSize()` before and after switching to static allocation.

Expected: free heap increases by approximately `sizeof(StaticTask_t) + 256*4` bytes (the TCB and stack no longer come from the heap).

**Checkpoint D:** UART output showing free heap before and after the conversion.

> **Question D1:** With `configSUPPORT_STATIC_ALLOCATION = 1` and `configSUPPORT_DYNAMIC_ALLOCATION = 0` (all static), what are the benefits for a safety-critical system? What does the linker map tell you that `xPortGetFreeHeapSize()` cannot?

> **Question D2:** `vApplicationGetIdleTaskMemory` must be implemented when static allocation is enabled. Why does the idle task require static allocation even if all your application tasks use dynamic allocation?

---

## Deliverables

| # | Item |
|---|------|
| 1 | Part A1 UART capture — crash without detection |
| 2 | Part A2 UART capture — stack overflow hook message |
| 3 | Part B stack high-water-mark table (all Lab 03 tasks + overflow task) |
| 4 | Part C linker map section size table |
| 5 | Part D UART capture — heap usage before and after static conversion |
| 6 | Written answers to Questions A1–A2, B1–B2, D1–D2 |

---

## Key Concepts Demonstrated

| Concept | Where you saw it |
|---------|-----------------|
| Stack overflow → silent corruption | Part A (no detection) |
| `vApplicationStackOverflowHook` | Part A (with detection) |
| High-water-mark measurement and right-sizing | Part B |
| Heap usage accounting | Part C |
| Linker map section analysis | Part C |
| Static task allocation (`xTaskCreateStatic`) | Part D |

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| Barry, *Mastering the FreeRTOS Kernel* | Ch. 3 (heap schemes) |
| FreeRTOS docs | `configCHECK_FOR_STACK_OVERFLOW`, `uxTaskGetStackHighWaterMark`, `xTaskCreateStatic` |
| NXP MCXN236 Reference Manual | §SRAM memory map |
