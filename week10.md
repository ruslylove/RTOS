---
theme: seriph
background: https://cover.sli.dev
title: "RTOS · Week 10 — FreeRTOS Memory Management"
info: |
  ## Real-Time Operating Systems
  Week 10 — FreeRTOS Memory Management

  KMUTNB · Faculty of Engineering · Department of Electrical and Computer Engineering
  Master of Engineering (M.Eng.) — Electrical and Computer Engineering
class: text-center
transition: slide-left
mdc: true
routeAlias: week-10
fonts:
  sans: Inter
  serif: Lora
  mono: Fira Code
---

# Real-Time Operating Systems

## Week 10 — FreeRTOS Memory Management

heap_1–heap_5 · stack sizing · MPU · static allocation

<div class="pt-10 opacity-70 text-sm">
  KMUTNB · Faculty of Engineering · M.Eng. in Electrical & Computer Engineering
</div>

<div class="abs-br m-6 text-xs opacity-50">
  Reading: FreeRTOS Book Ch. 3 · NXP MCXN236 RM §SRAM
</div>

---
layout: two-cols
layoutClass: gap-8
---

# From Week 9 to Week 10

We can now measure Cᵢ with cycle accuracy.

But where does the memory for tasks, queues, and stacks come from?

In embedded systems there is:
- **No MMU** — physical addresses only
- **No virtual memory** — what you see is what you get
- **No OS-managed page cache** — stack overflow → corruption

::right::

<div class="mt-10 px-5 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-sm leading-relaxed">

**This week — manage embedded memory safely:**

- FreeRTOS heap schemes: heap_1 to heap_5
- Stack sizing and overflow detection
- Memory Protection Unit (MPU) for hardware isolation
- Static allocation — the certification path
- Lab 7: stack overflow investigation

</div>

---

# Week 10 — Learning Objectives

By the end of this lecture you will be able to:

<v-clicks>

- **Describe** each FreeRTOS heap scheme and select the appropriate one for a given application.
- **Size** task stacks using `uxTaskGetStackHighWaterMark` and `configCHECK_FOR_STACK_OVERFLOW`.
- **Configure** the MPU to isolate task memory regions.
- **Implement** static allocation for tasks and queues with no runtime heap usage.
- **Diagnose** a stack overflow using the overflow hook and high-water mark.

</v-clicks>

<div v-click class="mt-6 px-4 py-2 border-l-4 border-amber-500 bg-amber-50 dark:bg-amber-900/20 text-sm">
Maps to <b>CLO 4</b> — analyse and verify memory usage in resource-constrained RTOS applications.
</div>

---
layout: section
---

# Part 1
## Memory in Embedded Systems

---
layout: statement
---

# No MMU — No Safety Net

In a desktop OS, a bad pointer faults safely. In an embedded system, it silently corrupts your neighbour's stack.

<div class="mt-8 text-base opacity-80 max-w-2xl mx-auto">
SRAM on MCXN236: 512 KB total. Every task stack, every queue buffer, every TCB comes from here. No swapping, no copy-on-write, no guard pages by default. Memory discipline is your responsibility.
</div>

---

# MCXN236 Memory Map (Simplified)

<div class="mt-4 text-sm">

| Region | Address range | Size | Use |
|--------|--------------|------|-----|
| Flash (NS alias) | 0x0000_0000 | 2 MB | Code, const data |
| SRAM0 | 0x2000_0000 | 128 KB | Stack, heap (main) |
| SRAM1 | 0x2002_0000 | 128 KB | Stack, heap (overflow) |
| SRAM2 | 0x2004_0000 | 128 KB | General |
| SRAM3 | 0x2006_0000 | 128 KB | General |
| SRAM4 | 0x2008_0000 | 128 KB | Shared between CM33_0 and CM33_1 |

</div>

<div v-click class="mt-5 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
Use <b>heap_5</b> to span multiple SRAM banks, maximising allocatable heap without a single contiguous requirement.
</div>

---
layout: section
---

# Part 2
## FreeRTOS Heap Schemes

---

# heap_1 — Allocate Only

The simplest scheme: a static array divided by a pointer. **No free.**

```c
/* FreeRTOSConfig.h */
#define configTOTAL_HEAP_SIZE   ( 64 * 1024 )

/* All allocations come from ucHeap[] — never freed */
static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
```

<div class="mt-4 grid grid-cols-2 gap-4 text-sm">
<div class="px-4 py-2 rounded bg-green-50 dark:bg-green-900/30">
<b>Use when:</b> task set is fully fixed at startup. Tasks, queues, semaphores are created once and never deleted. Deterministic — O(1) allocation.
</div>
<div class="px-4 py-2 rounded bg-amber-50 dark:bg-amber-900/30">
<b>Limitation:</b> calling `vPortFree()` does nothing. No dynamic task creation after startup. Simplest to certify — total memory usage is statically known.
</div>
</div>

---

# heap_2, heap_3, heap_4, heap_5

<div class="mt-3 text-sm">

| Scheme | Algorithm | Free? | Coalescence | Use case |
|--------|----------|-------|------------|---------|
| **heap_2** | Best-fit | Yes | No | Deprecated. Use heap_4. |
| **heap_3** | stdlib wrapper | Yes | stdlib-dependent | Existing malloc/free. Non-deterministic. |
| **heap_4** | First-fit | Yes | Yes | Most common. Dynamic tasks. Single array. |
| **heap_5** | First-fit | Yes | Yes | Multiple non-contiguous SRAM banks. |

</div>

<div class="mt-4 text-sm">

**heap_4 in practice:**

```c
#define configTOTAL_HEAP_SIZE   ( 128 * 1024 )
/* heap_4 manages ucHeap automatically. No init needed. */
```

**heap_5 — span two SRAM banks:**

```c
static uint8_t bank0_heap[64*1024];  /* in SRAM0 */
static uint8_t bank1_heap[64*1024];  /* in SRAM1 */

HeapRegion_t xHeapRegions[] = {
    { bank0_heap, sizeof(bank0_heap) },
    { bank1_heap, sizeof(bank1_heap) },
    { NULL, 0 }
};
/* Call once before any allocation */
vPortDefineHeapRegions( xHeapRegions );
```

</div>

---
layout: section
---

# Part 3
## Stack Management

---
layout: two-cols
layoutClass: gap-6
---

# Stack Sizing

Every task created with `xTaskCreate` gets its own stack allocated from the heap.

```c
/* stackDepth in WORDS (4 bytes each), not bytes */
xTaskCreate(
    vMyTask,
    "MyTask",
    256,        /* 256 words = 1024 bytes */
    NULL,
    2,
    &xHandle
);
```

**Too small → stack overflow → heap/TCB corruption → undefined behaviour.**

**How to right-size:**

```c
/* After system has run for a while: */
UBaseType_t watermark =
    uxTaskGetStackHighWaterMark(xHandle);
/* watermark = minimum free words remaining.
   0 = stack has already overflowed! */
```

::right::

<div class="mt-6 px-5 py-4 rounded-lg bg-amber-50 dark:bg-amber-900/30 text-sm leading-relaxed">

### Stack overflow detection

**Method 1** (`configCHECK_FOR_STACK_OVERFLOW = 1`):

At each context switch, check if stack pointer has gone below the stack start. Fast, but misses overflows that occur and recover mid-task.

**Method 2** (`configCHECK_FOR_STACK_OVERFLOW = 2`):

Fill the entire stack with a known pattern (0xA5). At each switch, check if the last 20 bytes are still the pattern. Catches most overflows.

```c
void vApplicationStackOverflowHook(
    TaskHandle_t xTask,
    char *pcTaskName)
{
    /* Log, blink LED, reset */
    taskDISABLE_INTERRUPTS();
    for(;;) {}
}
```

</div>

---
layout: section
---

# Part 4
## Memory Protection Unit

---

# MPU on Cortex-M33

The MPU provides **hardware-enforced isolation** between memory regions. Cortex-M33 has 8 MPU regions.

<div class="mt-4 text-sm">

Each region defines:
- Base address and size (power-of-2 aligned)
- Access permissions: Privileged R/W, Unprivileged R/W, Read-only, No access
- Execute Never (XN): prevents code execution from data regions (stack, heap)

</div>

```c
/* FreeRTOS MPU port — define per-task regions */
static const MemoryRegion_t xTaskRegions[] = {
    /* Base addr,  Size,                         Params */
    { 0x20000000,  4096, portMPU_REGION_READ_WRITE },
    { 0x00000000,  0,    0 },   /* end */
    { 0x00000000,  0,    0 }
};

/* Create MPU-protected task */
xTaskCreateRestricted(
    &xTaskDefinition,  /* includes stack + regions */
    &xHandle
);
```

<div v-click class="mt-3 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
MPU + FreeRTOS: each task runs with restricted access — a buggy task cannot corrupt another task's stack or peripheral registers. Requires <code>portUSING_MPU_WRAPPERS = 1</code>.
</div>

---
layout: section
---

# Part 5
## Static Allocation

---

# Static Allocation — No Runtime Heap

`xTaskCreateStatic` and `xQueueCreateStatic` require pre-allocated buffers at compile time.

```c {all|1-7|9-18}{maxHeight:'300px'}
/* Buffers declared at file scope — known at link time */
static StaticTask_t  xTaskBuffer;
static StackType_t   xStack[256];    /* 256 words = 1 KB */
static StaticQueue_t xQueueBuffer;
static uint8_t       xQueueStorage[10 * sizeof(uint32_t)];

TaskHandle_t xHandle;

/* Create task with static buffers */
xHandle = xTaskCreateStatic(
    vMyTask,
    "MyTask",
    256,            /* stack depth (words) */
    NULL,           /* pvParameters */
    2,              /* priority */
    xStack,         /* stack buffer */
    &xTaskBuffer    /* TCB buffer */
);

/* Create queue with static buffer */
QueueHandle_t xQ = xQueueCreateStatic(
    10,                   /* length */
    sizeof(uint32_t),     /* item size */
    xQueueStorage,        /* storage buffer */
    &xQueueBuffer         /* queue state */
);
```

<div v-click class="mt-3 text-sm px-4 py-2 border-l-4 border-green-500 bg-green-50 dark:bg-green-900/20">
With static allocation: zero heap usage at runtime. Total memory is fixed at link time — verifiable by the linker map file. Required for DO-178C and IEC 61508 SIL-3+.
</div>

---
layout: section
---

# Part 6
## Lab 7 — Stack Overflow Investigation

---
layout: two-cols
layoutClass: gap-6
---

# Lab 7 — Find and Fix the Overflow

<v-clicks>

**Step 1 — Induce overflow:**
1. Create a task with `stackDepth = 64` (very small)
2. Call a recursive function inside the task — eventually overflows
3. Observe corruption (wrong output, fault, or random reset)

**Step 2 — Detect overflow:**
4. Enable `configCHECK_FOR_STACK_OVERFLOW = 2`
5. Implement `vApplicationStackOverflowHook` — blink LED + print task name
6. Reproduce the overflow — hook fires before corruption spreads

**Step 3 — Right-size:**
7. Increase stack to 512 words, run for 60 seconds
8. `uxTaskGetStackHighWaterMark` — record minimum free
9. Set final stack = (512 - highwatermark) + 20 % margin

</v-clicks>

::right::

<div class="mt-8 px-5 py-4 rounded-lg bg-amber-50 dark:bg-amber-900/30 text-sm leading-relaxed">

**What to submit**

- Stack high-water-mark table for all tasks in your Lab 3 system
- Total heap usage: `configTOTAL_HEAP_SIZE` − `xPortGetFreeHeapSize()`
- Linker map analysis: size of `.bss`, `.data`, `.text`
- Optional: convert one task to static allocation and verify heap usage drops

<div class="mt-3 text-xs opacity-70">
Reading — FreeRTOS book Ch. 3 · configCHECK_FOR_STACK_OVERFLOW docs
</div>

</div>

---
layout: default
---

# Key Takeaways

<v-clicks>

- Embedded systems have **no MMU** — stack overflows silently corrupt adjacent memory. Prevention and detection are your responsibility.
- **heap_1**: allocation-only, fully deterministic — use for fixed task sets. **heap_4**: first-fit with coalescence — most common. **heap_5**: spans multiple SRAM regions — use on MCXN236 to leverage all 512 KB.
- **Stack sizing**: start large, measure high-water-mark, shrink + add 20 % margin. Always implement `vApplicationStackOverflowHook`.
- The **MPU** provides hardware isolation between tasks — a fault in one task cannot corrupt another's stack or peripheral registers. Requires FreeRTOS MPU port.
- **Static allocation** (`xTaskCreateStatic`) eliminates runtime heap use entirely — required for safety certification and deterministic memory analysis.

</v-clicks>

<div v-click class="mt-5 text-center text-base px-4 py-2 rounded bg-blue-100 dark:bg-blue-900/40">
Module 4 complete. Next — <b>Module 5: Advanced Hardware</b>. Week 11: ARM TrustZone-M security.
</div>

---

# Before Next Week

<div class="grid grid-cols-2 gap-8 mt-6">

<div>

### Reading
- **FreeRTOS Book**, Ch. 3 — heap schemes
- **NXP MCXN236 Reference Manual** — SRAM chapter, MPU configuration
- Review: ARM Cortex-M33 MPU programming model (TRM §B3.5)

### Lab
- Complete **Lab 7** — stack overflow + high-water-mark table
- Convert the sensor task from Lab 3 to static allocation

</div>

<div>

### Check yourself
<div class="text-sm">

1. A FreeRTOS application creates 5 tasks and 3 queues, then deletes 2 tasks. Which heap scheme(s) handle this correctly? Which do not?
2. `uxTaskGetStackHighWaterMark` returns 4 for a task. What does this mean? Is the task safe?
3. You have 256 KB of SRAM at 0x2000_0000 and 128 KB at 0x2010_0000 (non-contiguous). Which heap scheme do you use? Sketch the `vPortDefineHeapRegions` call.
4. With MPU enabled, a task attempts to write to address 0x0800_0000 (flash). What happens?

</div>

</div>

</div>

---
layout: end
class: text-center
---

# Week 10 Complete

FreeRTOS Memory Management

<div class="mt-4 text-sm opacity-70">
Real-Time Operating Systems · KMUTNB · M.Eng. ECE<br/>
Next — Week 11 · ARM TrustZone-M Security
</div>

<style>
:root { --slidev-theme-primary: #003874; }
.slidev-layout h1 { color: #003874; }
.dark .slidev-layout h1 { color: #7ba7d9; }
table { font-size: 0.92em; }
</style>
