---
theme: seriph
background: https://cover.sli.dev
title: "RTOS · Week 6 — Queues, Semaphores & Mutexes"
info: |
  ## Real-Time Operating Systems
  Week 6 — Queues, Semaphores & Mutexes

  KMUTNB · Faculty of Engineering · Department of Electrical and Computer Engineering
  Master of Engineering (M.Eng.) — Electrical and Computer Engineering
class: text-center
transition: slide-left
mdc: true
routeAlias: week-6
fonts:
  sans: Inter
  serif: Lora
  mono: Fira Code
---

# Real-Time Operating Systems

## Week 6 — Queues, Semaphores &amp; Mutexes

Race conditions · FreeRTOS IPC primitives · event groups · Lab 3

<div class="pt-10 opacity-70 text-sm">
  KMUTNB · Faculty of Engineering · M.Eng. in Electrical & Computer Engineering
</div>

<div class="abs-br m-6 text-xs opacity-50">
  Reading: Barry Ch. 4–5 · FreeRTOS Reference Manual
</div>

---
layout: two-cols
layoutClass: gap-8
---

# From Week 5 to Week 6

Module 2 is complete — we can schedule tasks with **timing guarantees**.

Now Module 3: tasks don't just run, they **communicate** and **share resources**.

- How does a sensor task pass data to a control task?
- How does one task signal another to wake up?
- How do two tasks safely share a hardware peripheral?

::right::

<div class="mt-10 px-5 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-sm leading-relaxed">

**This week — the FreeRTOS IPC toolkit:**

- Race conditions and why they matter
- **Queues**: safe data transfer between tasks
- **Semaphores**: signalling (binary, counting)
- **Mutexes**: mutual exclusion with ownership
- **Event groups**: multi-condition synchronisation

<div class="mt-3 opacity-80">
Every synchronisation problem you encounter will map onto one of these primitives.
</div>

</div>

---

# Week 6 — Learning Objectives

By the end of this lecture you will be able to:

<v-clicks>

- **Explain** what a race condition is and why a critical section is needed.
- **Create and use** FreeRTOS queues for task-to-task data passing.
- **Distinguish** binary semaphores, counting semaphores, and mutexes by purpose and API.
- **Select** the correct primitive for a given synchronisation pattern.
- **Use** event groups for multi-task rendezvous.
- **Implement** a producer-consumer pipeline with queues on the FRDM-MCXN236.

</v-clicks>

<div v-click class="mt-6 px-4 py-2 border-l-4 border-amber-500 bg-amber-50 dark:bg-amber-900/20 text-sm">
Maps to <b>CLO 2</b> — <i>Design and implement real-time task sets on an RTOS, meeting specified timing constraints.</i>
</div>

---
layout: section
---

# Part 1
## Why Shared Data Is Dangerous

---
layout: statement
---

# The Race Condition

Two tasks reading, modifying, and writing the same variable **without synchronisation** can corrupt it.

<div class="mt-8 text-base opacity-80 max-w-2xl mx-auto">
Preemption can occur between any two instructions. A 32-bit read-modify-write is
<b>not atomic</b> on a load-store architecture. The window is tiny but real — and in
production systems, it is eventually hit.
</div>

---

# A Race in Action

Two tasks increment a shared counter 1000 times each — expected result: 2000.

```c {all|1-7|9-16|all}
static volatile uint32_t gCounter = 0;

void vTaskA(void *pv) {
    for (int i = 0; i < 1000; i++)
        gCounter++;          /* LDR + ADD + STR — three instructions! */
}

void vTaskB(void *pv) {
    for (int i = 0; i < 1000; i++)
        gCounter++;
}
/* Actual result: typically 1000–2000 — non-deterministic */
```

<div v-click class="mt-4 grid grid-cols-2 gap-4 text-sm">
<div class="px-3 py-2 rounded bg-red-50 dark:bg-red-900/30">

Task A reads gCounter = 500. Task A is **preempted**. Task B runs, gCounter = 501. Task A resumes, writes 501 (not 502). **One increment lost.**

</div>
<div class="px-3 py-2 rounded bg-green-50 dark:bg-green-900/30">

Fix: protect with a mutex or use an atomic operation. Or design to avoid shared state entirely — use **queues** to pass data by value.

</div>
</div>

---

# Critical Sections in FreeRTOS

`taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` — raise BASEPRI to mask all interrupts up to `configMAX_SYSCALL_INTERRUPT_PRIORITY`.

```c
taskENTER_CRITICAL();
gCounter++;           /* now atomic with respect to tasks and masked IRQs */
taskEXIT_CRITICAL();
```

<div class="mt-4 grid grid-cols-2 gap-6 text-sm">

<div class="px-4 py-3 rounded bg-amber-50 dark:bg-amber-900/30">
<div class="font-bold">When to use</div>
<div class="mt-1">Very short, deterministic code sequences (one or two memory accesses). Timer ISRs are the primary concern.</div>
</div>

<div class="px-4 py-3 rounded bg-red-50 dark:bg-red-900/30">
<div class="font-bold">When NOT to use</div>
<div class="mt-1">Long sections, anything that can block, loops. Holding a critical section while waiting adds interrupt latency — violates the real-time model.</div>
</div>

</div>

<div v-click class="mt-4 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
For most data sharing, prefer <b>queues</b> or <b>mutexes</b> — they work with the scheduler rather than around it.
</div>

---
layout: section
---

# Part 2
## Queues

---
layout: two-cols
layoutClass: gap-6
---

# FreeRTOS Queues

A queue is a **thread-safe, blocking FIFO** that copies items by value. The fundamental IPC mechanism in FreeRTOS.

```c
/* Create a queue of 10 int32_t items */
QueueHandle_t xQ = xQueueCreate(10, sizeof(int32_t));

/* Send (blocks if full) */
int32_t val = 42;
xQueueSendToBack(xQ, &val, portMAX_DELAY);

/* Receive (blocks if empty) */
int32_t rx;
xQueueReceive(xQ, &rx, portMAX_DELAY);
```

Key properties:
- **Copy semantics** — the item is copied in and out. No pointer sharing required.
- **Blocking** — sender blocks when full; receiver blocks when empty.
- **ISR-safe** — use `xQueueSendFromISR` / `xQueueReceiveFromISR`.

::right::

<div class="mt-10 px-5 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-sm leading-relaxed">

### Queue of structs

For multi-field messages, define a struct and queue it:

```c
typedef struct {
    uint32_t timestamp;
    float    temperature;
} SensorData_t;

QueueHandle_t xSensorQ =
    xQueueCreate(5, sizeof(SensorData_t));

/* Sender */
SensorData_t msg = { xTaskGetTickCount(), 36.5f };
xQueueSendToBack(xSensorQ, &msg, 0);

/* Receiver */
SensorData_t rx;
xQueueReceive(xSensorQ, &rx, portMAX_DELAY);
```

</div>

---

# Queue Semantics and Priority

When a task blocks waiting to send or receive, FreeRTOS orders the waiting tasks by **priority**. The highest-priority waiter is unblocked first.

<div class="mt-4 text-sm">

| Function | Blocking | ISR-safe |
|----------|----------|----------|
| `xQueueSendToBack(q, &item, ticks)` | Yes | No |
| `xQueueSendToFront(q, &item, ticks)` | Yes | No |
| `xQueueSendToBackFromISR(q, &item, &woken)` | No | Yes |
| `xQueueReceive(q, &buf, ticks)` | Yes | No |
| `xQueuePeek(q, &buf, ticks)` | Yes — copy without remove | No |
| `uxQueueMessagesWaiting(q)` | No | No |

</div>

<div v-click class="mt-4 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
Use <code>xQueueSendToFront</code> for high-priority messages that should jump the queue — e.g. an emergency stop command.
</div>

---
layout: section
---

# Part 3
## Semaphores

---

# Binary and Counting Semaphores

Semaphores are **signalling** tools — not data carriers. The value is 0 (not available) or ≥1 (available).

<div class="mt-4 grid grid-cols-2 gap-6 text-sm">

<div class="px-4 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30">
<div class="text-base font-bold text-blue-700 dark:text-blue-300">Binary semaphore</div>

```c
SemaphoreHandle_t xSem =
    xSemaphoreCreateBinary();

/* ISR: signal the task */
xSemaphoreGiveFromISR(xSem, &woken);

/* Task: wait for signal */
xSemaphoreTake(xSem, portMAX_DELAY);
```

Max count = 1. Classic use: ISR → task synchronisation.
</div>

<div class="px-4 py-4 rounded-lg bg-amber-50 dark:bg-amber-900/30">
<div class="text-base font-bold text-amber-700 dark:text-amber-300">Counting semaphore</div>

```c
SemaphoreHandle_t xCount =
    xSemaphoreCreateCounting(
        3,  /* max count */
        3   /* initial count */
    );

/* Acquire a resource slot */
xSemaphoreTake(xCount, portMAX_DELAY);
/* use resource */
xSemaphoreGive(xCount);
```

Models a pool of $n$ identical resources.
</div>

</div>

<div v-click class="mt-4 text-sm px-4 py-2 border-l-4 border-amber-500 bg-amber-50 dark:bg-amber-900/20">
Binary semaphore vs. mutex: both block, but the mutex has <b>ownership</b> and <b>priority inheritance</b>. Use a semaphore for signalling; use a mutex for exclusion.
</div>

---
layout: section
---

# Part 4
## Mutexes

---
layout: two-cols
layoutClass: gap-6
---

# FreeRTOS Mutex

A mutex is a binary semaphore with **ownership** (only the taker can give it back) and **priority inheritance** (Week 7 topic).

```c
SemaphoreHandle_t xMutex =
    xSemaphoreCreateMutex();

void vTaskA(void *pv) {
    for (;;) {
        xSemaphoreTake(xMutex,
                       portMAX_DELAY);
        /* protected section */
        vAccessSharedHardware();
        xSemaphoreGive(xMutex);
    }
}
```

::right::

<div class="mt-6 px-5 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-sm leading-relaxed">

### Rules for FreeRTOS mutexes

- **Cannot be given from an ISR** — ISRs have no task context, so ownership is undefined
- **Must be given by the same task that took it** — enforced by the ownership model
- **configUSE_MUTEXES must be 1** in FreeRTOSConfig.h
- **Recursive mutex** (`xSemaphoreCreateRecursiveMutex`): same task can take multiple times without blocking — must give the same number of times

<div class="mt-3 opacity-80">
Use mutexes to protect <b>shared peripherals</b> (UART, SPI, I2C) and <b>shared data structures</b> when the critical section is longer than a few instructions.
</div>

</div>

---

# Choosing the Right Primitive

<div class="mt-4 text-sm">

| Pattern | Primitive | Why |
|---------|----------|-----|
| ISR notifies task of event | Binary semaphore | ISR-safe, lightweight |
| Task-to-task data passing | Queue | Copy semantics, thread-safe |
| Protect shared resource | Mutex | Ownership + priority inheritance |
| Resource pool (n items) | Counting semaphore | Count tracks availability |
| Wait for multiple events | Event group | Bit-mask, wait ANY or ALL |
| Short atomic memory access | Critical section | Cheapest, but masks interrupts |

</div>

<div v-click class="mt-5 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
When in doubt: model the communication pattern first (signal? data? exclusion?), then pick the primitive. Mismatching primitives is a common source of subtle bugs.
</div>

---
layout: section
---

# Part 5
## Event Groups

---

# Event Groups

An event group holds **24 bits** (on a 32-bit port). Tasks can set bits, clear bits, and wait for ANY or ALL specified bits.

```c {all|1-3|5-10|12-18}{maxHeight:'300px'}
#define EVT_SENSOR_READY   (1 << 0)
#define EVT_BUTTON_PRESSED (1 << 1)
#define EVT_COMMS_DONE     (1 << 2)

EventGroupHandle_t xEvents = xEventGroupCreate();

/* Sensor task: signal ready */
xEventGroupSetBits(xEvents, EVT_SENSOR_READY);

/* Comms task: signal done */
xEventGroupSetBits(xEvents, EVT_COMMS_DONE);

/* Control task: wait for BOTH sensor AND comms */
EventBits_t bits = xEventGroupWaitBits(
    xEvents,
    EVT_SENSOR_READY | EVT_COMMS_DONE,  /* bits to wait for */
    pdTRUE,                              /* clear on exit */
    pdTRUE,                              /* wait for ALL */
    portMAX_DELAY
);
```

<div v-click class="mt-3 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
Event groups are perfect for <b>rendezvous</b> patterns: multiple tasks must reach a synchronisation point before any continues.
</div>

---
layout: section
---

# Part 6
## Lab 3 — Producer-Consumer with Queues

---
layout: two-cols
layoutClass: gap-6
---

# Lab 3 — Producer-Consumer

Implement a two-producer, one-consumer pipeline.

<v-clicks>

**Setup:**
1. **vSensor1Task** (T=100 ms): reads ADC temperature, sends to queue
2. **vSensor2Task** (T=200 ms): reads a dummy voltage value, sends to queue
3. **vLoggerTask** (lowest priority): receives from queue, prints via UART

**Observations:**
4. Observe that the logger never misses data (queue buffers bursts)
5. Intentionally overfill the queue — what happens to the sender?
6. Add an event group: logger signals "queue drained" every 10 items

</v-clicks>

::right::

<div class="mt-10 px-5 py-4 rounded-lg bg-amber-50 dark:bg-amber-900/30 text-sm leading-relaxed">

**Queue sizing rule of thumb**

Queue depth = max burst size × number of senders ÷ consumer rate ratio.

For two sensors producing at 10 Hz + 5 Hz, consumed at any speed: a depth of 5–10 items handles all practical bursts.

<div class="mt-3 opacity-80">
Too small → senders block; too large → wastes heap. Use `uxQueueMessagesWaiting()` to measure high-water mark.
</div>

<div class="mt-3 text-xs opacity-70">
Reading — Barry Ch. 4 · FreeRTOS Queue API Reference
</div>

</div>

---
layout: default
---

# Key Takeaways

<v-clicks>

- A **race condition** corrupts shared data when a preemption happens between a read and a write. Protect with synchronisation, or avoid sharing via **queues**.
- **Queues** copy data by value — the safest way to pass information between tasks. Blocking on full/empty is intentional back-pressure.
- **Binary semaphores** signal events (ISR → task); **counting semaphores** model resource pools; **mutexes** protect shared resources with ownership.
- **Mutexes** add priority inheritance — use them (not binary semaphores) whenever an ISR could be affected by priority inversion.
- **Event groups** enable multi-condition rendezvous with AND/OR semantics across up to 24 flags.
- The FreeRTOS critical section (`taskENTER/EXIT_CRITICAL`) is for very short atomic sequences only.

</v-clicks>

<div v-click class="mt-5 text-center text-base px-4 py-2 rounded bg-blue-100 dark:bg-blue-900/40">
Next week — <b>Priority Inversion</b>: what goes wrong when mutexes and priorities interact badly, and three protocols that fix it.
</div>

---

# Before Next Week

<div class="grid grid-cols-2 gap-8 mt-6">

<div>

### Reading
- **Barry**, Ch. 4–5 — queues and semaphores
- **FreeRTOS Reference Manual** — xQueueCreate, xSemaphoreCreateMutex API pages

### Lab
- Complete **Lab 3 — Producer-Consumer**
- Try both blocking and non-blocking (timeout = 0) send modes
- Monitor heap usage with `xPortGetFreeHeapSize()`

</div>

<div>

### Check yourself
<div class="text-sm">

1. A binary semaphore is initialised to 0. Task A calls `xSemaphoreTake(pdMS_TO_TICKS(100))`. Task B calls `xSemaphoreGive()` 50 ms later. What happens to Task A?
2. Can you use `xSemaphoreGive()` from an ISR to unblock a task? Why or why not?
3. When does a queue prefer to block the **sender** rather than the receiver?
4. A counting semaphore is created with max=3, initial=3. Three tasks each take it. A fourth task tries to take it — what happens? What must happen before the fourth task unblocks?

</div>

</div>

</div>

---
layout: end
class: text-center
---

# Week 6 Complete

Queues, Semaphores &amp; Mutexes

<div class="mt-4 text-sm opacity-70">
Real-Time Operating Systems · KMUTNB · M.Eng. ECE<br/>
Next — Week 7 · Priority Inversion; PIP / PCP / SRP
</div>

<style>
:root { --slidev-theme-primary: #003874; }
.slidev-layout h1 { color: #003874; }
.dark .slidev-layout h1 { color: #7ba7d9; }
table { font-size: 0.92em; }
</style>
