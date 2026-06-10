# Lab 09 — Producer-Consumer IPC with FreeRTOS Queue

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** NXP FRDM-MCXN236 (Cortex-M33)  
**Estimated time:** 3–4 hours  
**Builds on:** Lab 06 (DWT), Lab 05 (watchdog / task coordination)

---

## Objectives

By completing this lab you will be able to:

1. **Implement** a lock-free single-producer / single-consumer ring buffer with correct memory barriers.
2. **Notify** a consumer task from a producer task using a FreeRTOS queue.
3. **Apply** the producer-consumer pattern to a simulated 100 Hz ADC data stream.
4. **Apply** a 4-tap moving-average FIR filter in the consumer task.
5. **Measure** IPC notification latency (queue send to consumer wake-up) using DWT timestamps.
6. **Analyse** throughput, dropped-sample rate, and CPU load.

> **Hardware note:** The MCXN236 is a single-core device. This lab demonstrates the same producer-consumer and ring-buffer concepts that appear in dual-core AMP systems, using two FreeRTOS tasks and a FreeRTOS queue as the notification mechanism instead of a hardware Messaging Unit (MU).

---

## Prerequisites

- [ ] Lab 05 complete (FreeRTOS task coordination).
- [ ] Lab 06 complete (DWT cycle counter working).
- [ ] ARM GNU Toolchain and `pyocd` working.

---

## Background

### Lock-free ring buffer

The ring buffer in `shared_mem.h` is safe for a **single producer / single consumer** without a mutex, provided:

1. The producer writes data **before** updating `write_idx`.
2. The consumer reads `write_idx` **before** reading data.
3. Both accesses use `volatile` to prevent compiler reordering.
4. `__DMB()` barriers are placed around the index updates to prevent CPU store/load reordering.

```c
/* Producer */
buf[write_idx] = sample;
__DMB();                         /* store visible before index update */
write_idx = next;
__DMB();                         /* index update visible before queue send */
xQueueSend(g_notify_queue, &write_idx, 0);

/* Consumer (after receiving from queue) */
__DMB();                         /* ensure write_idx read before data read */
while (read_idx != write_idx) {
    process(buf[read_idx]);
    read_idx = (read_idx + 1) % SIZE;
}
```

### FreeRTOS queue as IPC notification

Instead of a hardware Messaging Unit, we use a small FreeRTOS queue (depth 8) to pass the current `write_idx` from the producer to the consumer. This decouples the two tasks while keeping the data path through the ring buffer:

- Producer: `xQueueSend(g_notify_queue, &write_idx, 0)` — non-blocking, drops if full.
- Consumer: `xQueueReceive(g_notify_queue, &dummy, pdMS_TO_TICKS(100))` — blocks until notified or timeout.

---

## Project Structure

```
lab09_dual_core/
├── cm33_0/                         ← single binary: both tasks run here
│   ├── Makefile
│   └── src/
│       ├── main_cm33_0.c           ← hardware init, queue create, task create
│       ├── adc_producer.c/.h       ← simulated ADC producer task
│       ├── FreeRTOSConfig.h
│       └── (includes ../cm33_1/src/ for fir_consumer)
├── cm33_1/
│   └── src/
│       ├── fir_consumer.c/.h       ← FIR consumer task (compiled into cm33_0 binary)
│       └── FreeRTOSConfig.h
└── shared/
    └── shared_mem.h                ← ring buffer + latency structs
```

Both `adc_producer.c` and `fir_consumer.c` are compiled into a single ELF and run as independent FreeRTOS tasks. The `g_notify_queue` global queue handle is declared in `main_cm33_0.c` and referenced as `extern` from both task files.

---

## Part A — Ring Buffer and Task Setup

### Step 1 — Shared data structures (`shared_mem.h`)

```c
#define RING_BUF_SIZE  256U   /* must be a power of two */

typedef struct {
    volatile uint32_t write_idx;            /* updated by producer */
    volatile uint32_t read_idx;             /* updated by consumer */
    volatile uint32_t missed;               /* dropped samples (buffer full) */
    int16_t           data[RING_BUF_SIZE];
} __attribute__((packed, aligned(4))) RingBuf_t;

typedef struct {
    volatile uint32_t send_cycles;   /* DWT timestamp just before queue send */
    volatile uint32_t recv_cycles;   /* DWT timestamp at consumer wake-up */
} __attribute__((packed)) LatencyTest_t;

extern RingBuf_t     g_ring;
extern LatencyTest_t g_latency;
```

### Step 2 — Create the queue and tasks (`main_cm33_0.c`)

```c
QueueHandle_t g_notify_queue;

int main(void)
{
    BOARD_InitHardware();
    PRINTF("\r\n=== RTOS Lab 09: Producer-Consumer IPC ===\r\n");

    g_ring.write_idx = g_ring.read_idx = g_ring.missed = 0U;

    /* IPC queue: depth 8, element = uint32_t write_idx */
    g_notify_queue = xQueueCreate(8U, sizeof(uint32_t));
    configASSERT(g_notify_queue != NULL);

    xTaskCreate(vADCProducerTask, "ADCProd", 512, NULL, 3, NULL);  /* prio 3 */
    xTaskCreate(vFIRConsumerTask, "FIRCons", 512, NULL, 2, NULL);  /* prio 2 */

    PRINTF("[MAIN] starting scheduler — free heap: %u bytes\r\n",
           (unsigned)xPortGetFreeHeapSize());
    vTaskStartScheduler();
    for (;;) {}
}
```

**Checkpoint A:** UART showing the startup banner and free heap size before the scheduler starts.

> **Question A1:** The producer runs at priority 3 and the consumer at priority 2. What happens to the consumer task the instant `xQueueSend` posts a notification — does it run immediately or wait until the producer yields? Explain using FreeRTOS preemption rules.

---

## Part B — ADC Producer Task

### Step 1 — Implement `vADCProducerTask`

```c
/* adc_producer.c */
extern QueueHandle_t g_notify_queue;

void vADCProducerTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(10);  /* 100 Hz */
    uint32_t sample_count = 0;

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        int16_t sample = simulate_adc_sample();  /* LFSR pseudo-random 12-bit */

        uint32_t next = (g_ring.write_idx + 1U) % RING_BUF_SIZE;
        if (next == g_ring.read_idx) {
            g_ring.missed++;
            continue;   /* buffer full — drop */
        }

        g_ring.data[g_ring.write_idx] = sample;
        __DMB();                                    /* store before index */
        g_ring.write_idx = next;
        __DMB();                                    /* index before queue send */

        /* Part C: record send timestamp */
        g_latency.send_cycles = DWT->CYCCNT;
        __DMB();

        uint32_t widx = g_ring.write_idx;
        xQueueSend(g_notify_queue, &widx, 0);       /* non-blocking */

        if (++sample_count % 100U == 0U)
            PRINTF("[ADC] produced %lu  missed=%lu  heap=%u\r\n",
                   (unsigned long)sample_count,
                   (unsigned long)g_ring.missed,
                   (unsigned)xPortGetFreeHeapSize());
    }
}
```

### Step 2 — Build and flash

```bash
cd cm33_0 && make && make flash
```

Observe the ADC messages every 1 second (100 samples × 10 ms period). Missed count should be 0 at this stage with only the producer running.

**Checkpoint B:** UART showing ADC producer printing every 1 s with `missed=0`.

> **Question B1:** The producer uses `vTaskDelayUntil` instead of `vTaskDelay`. What is the difference, and why does it matter for a 100 Hz periodic task?

---

## Part C — FIR Consumer Task

### Step 1 — Implement `vFIRConsumerTask`

```c
/* fir_consumer.c */
extern QueueHandle_t g_notify_queue;

static int32_t fir_filter(int16_t new_sample)
{
    static int16_t history[4] = {0};
    static uint8_t idx = 0;
    history[idx] = new_sample;
    idx = (uint8_t)((idx + 1U) % 4U);
    int32_t acc = 0;
    for (uint8_t i = 0; i < 4U; i++) acc += history[i];
    return acc / 4;  /* 4-tap moving average */
}

void vFIRConsumerTask(void *pvParameters)
{
    uint32_t consumed = 0;
    TickType_t report_tick = xTaskGetTickCount();

    for (;;)
    {
        uint32_t dummy;
        xQueueReceive(g_notify_queue, &dummy, pdMS_TO_TICKS(100));

        /* Part C: record receive timestamp */
        g_latency.recv_cycles = DWT->CYCCNT;

        __DMB();  /* ensure write_idx is visible */

        while (g_ring.read_idx != g_ring.write_idx) {
            int16_t raw      = g_ring.data[g_ring.read_idx];
            int32_t filtered = fir_filter(raw);
            g_ring.read_idx  = (g_ring.read_idx + 1U) % RING_BUF_SIZE;
            consumed++;
            (void)filtered;
        }

        TickType_t now = xTaskGetTickCount();
        if ((now - report_tick) >= pdMS_TO_TICKS(1000)) {
            report_tick = now;
            PRINTF("[FIR] consumed=%lu  missed=%lu  heap=%u\r\n",
                   (unsigned long)consumed,
                   (unsigned long)g_ring.missed,
                   (unsigned)xPortGetFreeHeapSize());

            if (g_latency.send_cycles != 0U) {
                uint32_t lat = g_latency.recv_cycles - g_latency.send_cycles;
                PRINTF("[FIR] IPC latency: %lu cycles (~%lu ns @ 150 MHz)\r\n",
                       (unsigned long)lat,
                       (unsigned long)(lat * 1000UL / 150UL));
            }
        }
    }
}
```

**Checkpoint C:** UART showing both producer and consumer messages with `missed=0` sustained over 10 seconds.

> **Question C1:** Remove both `__DMB()` calls on the producer side, rebuild, and run. Do you observe any incorrect FIR outputs or missed samples? (This may be platform-dependent — if the bug doesn't manifest, explain theoretically why it could.) Re-add the barriers before continuing.

> **Question C2:** The ring buffer uses `volatile` on the indices. Is `volatile` sufficient to guarantee correctness? What does `volatile` guarantee vs what `__DMB()` guarantees?

---

## Part D — Latency and Throughput Analysis

### Step 1 — IPC notification latency

After running for 10 seconds, observe the IPC latency printed by the consumer. This measures the time from `xQueueSend` in the producer to `xQueueReceive` returning in the consumer, expressed in DWT cycles.

> **Question D1:** Convert the measured IPC latency cycles to nanoseconds at 150 MHz. What are the components of this latency? (Hint: consider FreeRTOS scheduler overhead, priority difference between tasks, and context-switch cost.)

### Step 2 — Increase producer rate

Change the producer period from `pdMS_TO_TICKS(10)` (100 Hz) to `pdMS_TO_TICKS(1)` (1 kHz). Observe whether `missed` increases. Record results in the table below.

| Producer rate | Consumed/s | Missed/s | IPC latency (cycles) |
|--------------|-----------|---------|---------------------|
| 100 Hz       |           |         |                     |
| 1 kHz        |           |         |                     |

**Checkpoint D:** Filled table with measured values at both rates.

> **Question D2:** At 1 kHz, why might `missed` increase? Is the bottleneck the ring buffer, the FreeRTOS queue depth, or the consumer processing time? Explain how you would determine which.

---

## Deliverables

| # | Item |
|---|------|
| 1 | Part A UART — startup banner with free heap size |
| 2 | Part B UART — ADC producer printing at 100 Hz with `missed=0` |
| 3 | Part C UART — 10 seconds of producer + consumer with `missed=0` |
| 4 | Part D comparison table — 100 Hz vs 1 kHz |
| 5 | Written answers to Questions A1, B1, C1, C2, D1, D2 |
| 6 | `shared_mem.h` ring buffer struct |
| 7 | `adc_producer.c` and `fir_consumer.c` source listings |

---

## Key Concepts Demonstrated

| Concept | Where you saw it |
|---------|-----------------|
| Lock-free SPSC ring buffer | Part B & C |
| DMB memory barriers — store ordering | Part C `__DMB()` |
| FreeRTOS queue as IPC notification | Parts B–D |
| Producer-consumer task coordination | Parts B & C |
| IPC latency measurement with DWT | Part C |
| Throughput vs drop-rate trade-off | Part D |

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| ARM Cortex-M33 TRM | §A3.4 Memory ordering, §DMB |
| Barry, *Mastering the FreeRTOS Kernel* | `xQueueSend`, `xQueueReceive`, `vTaskDelayUntil` |
| NXP MCUXpresso SDK | `fsl_debug_console.h`, `BOARD_InitHardware` |
| Herlihy & Shavit, *The Art of Multiprocessor Programming* | Ch. 3 (concurrent queues, lock-free) |
