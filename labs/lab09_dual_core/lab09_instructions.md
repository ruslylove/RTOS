# Lab 09 — Dual-Core AMP: Messaging Unit & Shared SRAM

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** NXP FRDM-MCXN236 (Cortex-M33 × 2)  
**Estimated time:** 4–5 hours  
**Builds on:** Lab 06 (DWT), Lab 07 (memory analysis)

---

## Objectives

By completing this lab you will be able to:

1. **Boot** both CM33 cores of the MCXN236 from a single project using SYSCON boot address registers.
2. **Exchange** 32-bit messages between cores using the Messaging Unit (MU) mailboxes.
3. **Implement** a shared SRAM ring buffer with correct DMB memory barriers.
4. **Measure** inter-core IPC latency (MU send → ISR entry) using DWT timestamps on both cores.
5. **Compare** MU-direct vs ring-buffer throughput for high-rate data transfer.

---

## Prerequisites

- [ ] Lab 06 complete (DWT cycle counter working).
- [ ] ARM GNU Toolchain and `pyocd` working.
- [ ] MCUXpresso SDK for MCXN236 (for `fsl_mu.h` and multicore startup code).
- [ ] Understanding of memory barriers (`__DMB()`, `__DSB()`).

---

## Background

### MCXN236 AMP configuration

The MCXN236 has two independent Cortex-M33 cores (CM33_0 and CM33_1). In AMP mode:

- Each core runs its own independent firmware image with its own FreeRTOS instance.
- CM33_0 boots first (primary core). It writes the CM33_1 boot address and releases the reset.
- Communication uses the **Messaging Unit (MU)** — four 32-bit TX/RX registers per direction, hardware flow control.
- Shared data lives in **SRAM4** (0x2008_0000, 128 KB) — accessible by both cores.

### Memory barriers

The Cortex-M33 has a weakly ordered memory model. Without barriers, the CPU may reorder stores/loads before the MU send, causing the consumer to read stale data:

```c
/* Producer — on CM33_0 */
shared_buf[idx] = data;   /* store */
__DMB();                   /* all preceding stores visible before MU send */
MU_SendMsg(MUA, 0, idx);   /* notify consumer */

/* Consumer ISR — on CM33_1 */
uint32_t idx = MU_ReceiveMsg(MUB, 0);
__DMB();                   /* MU read complete before we read shared_buf */
process(shared_buf[idx]);
```

---

## Project Structure

```
lab09_dual_core/
├── cm33_0/                          ← Primary core firmware
│   ├── Makefile
│   ├── linker_cm33_0.ld             ← SRAM0+1 private, SRAM4 shared
│   └── src/
│       ├── main_cm33_0.c            ← boots CM33_1, creates ADC task
│       ├── adc_producer.c/.h        ← ADC sampling task
│       ├── shared_mem.h             ← shared ring buffer declaration
│       ├── FreeRTOSConfig.h
│       └── uart.h / uart.c          ← LPUART on CM33_0
├── cm33_1/                          ← Secondary core firmware
│   ├── Makefile
│   ├── linker_cm33_1.ld             ← SRAM2+3 private, SRAM4 shared
│   └── src/
│       ├── main_cm33_1.c            ← MU init, FreeRTOS start
│       ├── fir_consumer.c/.h        ← FIR filter task
│       ├── shared_mem.h             ← same shared ring buffer header
│       ├── FreeRTOSConfig.h
│       └── uart.h / uart.c          ← LPUART on CM33_1
└── flash_dual.sh                    ← flashes both images
```

---

## Part A — Boot Both Cores

### Step 1 — Release CM33_1 from CM33_0

In `cm33_0/src/main_cm33_0.c`, after hardware init and before starting the scheduler:

```c
#include "fsl_syscon.h"

void Boot_CM33_1(void)
{
    /* Write CM33_1's vector table address (= start of its flash region) */
    SYSCON->CPBOOT   = 0x00080000U;   /* CM33_1 flash start */
    SYSCON->CPUCTRL |= SYSCON_CPUCTRL_CPU1RSTEN_MASK;   /* assert reset */
    SYSCON->CPUCTRL &= ~SYSCON_CPUCTRL_CPU1RSTEN_MASK;  /* release reset */
    uart_printf("[CM0] CM33_1 released at 0x00080000\r\n");
}
```

### Step 2 — CM33_1 startup

CM33_1's `main_cm33_1.c` initialises its own hardware and announces its presence:

```c
int main(void)
{
    BOARD_InitHardware_CM33_1();
    uart_printf("[CM1] Core 1 running at %lu Hz\r\n",
                CLOCK_GetFreq(kCLOCK_CoreSysClk));
    /* ... FreeRTOS init ... */
    vTaskStartScheduler();
}
```

**Checkpoint A:** UART showing both `[CM0]` and `[CM1]` startup messages.

> **Question A1:** CM33_0 releases CM33_1 by writing `CPBOOT` and toggling `CPU1RSTEN`. What would happen if CM33_0 wrote `CPBOOT` but never cleared `CPU1RSTEN`?

---

## Part B — MU Direct Messaging

### Step 1 — Send 32-bit messages

On CM33_0 (sender task):

```c
void vMuSendTask(void *pv)
{
    uint32_t count = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
        MU_SendMsg(MUA, 0, count);
        uart_printf("[CM0] sent %lu\r\n", count);
        count++;
    }
}
```

On CM33_1 (MU receive ISR):

```c
void MUA_IRQHandler(void)
{
    uint32_t msg = MU_ReceiveMsg(MUB, 0);
    MU_ClearStatusFlags(MUB, kMU_Rx0FullFlag);
    uart_printf("[CM1] received %lu\r\n", msg);
}

/* In main_cm33_1.c: enable MU RX interrupt */
MU_EnableInterrupts(MUB, kMU_Rx0FullInterruptEnable);
EnableIRQ(MUA_IRQn);
```

**Checkpoint B:** UART showing matching send/receive sequence.

### Step 2 — Measure MU latency with DWT

On CM33_0, record the DWT cycle count just before `MU_SendMsg`. On CM33_1, record it in the first instruction of the ISR. Use a shared SRAM4 variable to communicate timestamps:

```c
/* shared_mem.h */
typedef struct {
    volatile uint32_t send_cycles;    /* written by CM33_0 */
    volatile uint32_t recv_cycles;    /* written by CM33_1 ISR */
} __attribute__((packed)) LatencyTest_t;

extern LatencyTest_t g_latency;   /* in .sram4 section */
```

```c
/* CM33_0 sender */
g_latency.send_cycles = DWT->CYCCNT;
__DMB();
MU_SendMsg(MUA, 0, 0xDEAD);

/* CM33_1 ISR — first two lines */
g_latency.recv_cycles = DWT->CYCCNT;
__DMB();
```

After a few exchanges, CM33_1 prints the latency:

```c
uint32_t latency = g_latency.recv_cycles - g_latency.send_cycles;
uart_printf("[CM1] MU latency = %lu cycles (%.1f ns @ 150 MHz)\r\n",
            latency, latency / 150.0f);
```

**Checkpoint B2:** Terminal showing measured MU latency in cycles and nanoseconds.

> **Question B1:** The MU TX register has hardware flow control — `MU_SendMsg` blocks until the remote core reads the register. For a high-frequency producer (1 kHz), is this acceptable? How would you fix it without blocking the sender?

---

## Part C — Shared SRAM Ring Buffer

### Step 1 — Place ring buffer in SRAM4

In `shared_mem.h`:

```c
#define RING_BUF_SIZE  256

typedef struct {
    volatile uint32_t write_idx;
    volatile uint32_t read_idx;
    volatile uint32_t missed;      /* overflow counter */
    int16_t           data[RING_BUF_SIZE];
} __attribute__((packed, aligned(4))) RingBuf_t;

/* Defined in a .sram4 section via linker script */
extern RingBuf_t g_ring;
```

In the linker script (`cm33_0/linker_cm33_0.ld`):

```ld
MEMORY {
  flash0  (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
  sram01  (rwx) : ORIGIN = 0x20000000, LENGTH = 256K   /* CM33_0 private */
  sram4   (rw)  : ORIGIN = 0x20080000, LENGTH = 128K   /* shared */
}

SECTIONS {
  /* ... normal sections ... */
  .sram4 (NOLOAD) : {
    KEEP(*(.sram4*))
  } > sram4
}
```

### Step 2 — Producer on CM33_0 (1 kHz ADC task)

```c
void vADCTask(void *pv)
{
    TickType_t xLastWake = xTaskGetTickCount();
    uint32_t sample_count = 0;
    for (;;) {
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(1));  /* 1 kHz */

        uint32_t next = (g_ring.write_idx + 1) % RING_BUF_SIZE;
        if (next == g_ring.read_idx) {
            g_ring.missed++;
            continue;   /* buffer full — drop sample */
        }

        /* Simulate ADC: ramp 0–4095 */
        g_ring.data[g_ring.write_idx] = (int16_t)(sample_count % 4096);
        sample_count++;
        __DMB();                        /* store before index update */
        g_ring.write_idx = next;
        __DMB();                        /* index update before MU notify */

        /* Notify CM33_1 every 16 samples (batched) */
        if ((g_ring.write_idx & 0xF) == 0)
            MU_TrySendMsg(MUA, 0, g_ring.write_idx);
    }
}
```

### Step 3 — Consumer on CM33_1 (FIR filter task)

```c
static int16_t fir_h[8] = {1, 2, 3, 4, 4, 3, 2, 1};   /* 8-tap symmetric FIR */
static int32_t fir_state[8] = {0};

void MUA_IRQHandler(void)
{
    MU_ReceiveMsg(MUB, 0);   /* consume notification */
    MU_ClearStatusFlags(MUB, kMU_Rx0FullFlag);
    xTaskNotifyGiveFromISR(xFIRHandle, NULL);
}

void vFIRTask(void *pv)
{
    uint32_t total = 0;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        __DMB();  /* ensure ring buffer data is visible */

        while (g_ring.read_idx != g_ring.write_idx) {
            int16_t sample = g_ring.data[g_ring.read_idx];
            g_ring.read_idx = (g_ring.read_idx + 1) % RING_BUF_SIZE;

            /* Simple FIR (shift register) */
            memmove(&fir_state[1], &fir_state[0], 7 * sizeof(int32_t));
            fir_state[0] = sample;
            int32_t acc = 0;
            for (int k = 0; k < 8; k++) acc += fir_state[k] * fir_h[k];
            total++;
        }

        if ((total % 1000) == 0)
            uart_printf("[CM1] processed %lu samples  missed=%lu\r\n",
                        total, g_ring.missed);
    }
}
```

**Checkpoint C:** UART from CM33_1 showing processed sample count and zero missed samples at 1 kHz.

> **Question C1:** Remove both `__DMB()` calls on the producer side, rebuild, and run. Do you observe incorrect FIR outputs or missed samples? (This may be platform-dependent — if the bug doesn't manifest, explain theoretically why it could.) Re-add the barriers before continuing.

> **Question C2:** The ring buffer uses `volatile` on the indices. Is `volatile` sufficient to guarantee correctness on a multi-core system? What does `volatile` guarantee vs what `__DMB()` guarantees?

---

## Part D — Comparison: MU Direct vs Ring Buffer

Run both configurations with a 1 kHz producer for 10 seconds. Measure:

| Metric | MU direct (Part B) | Ring buffer (Part C) |
|--------|-------------------|---------------------|
| Max throughput (samples/s) | | |
| Latency per sample (cycles) | | |
| Missed samples at 1 kHz | | |
| CM33_0 CPU load (%) | | |

Measure CM33_0 CPU load using `vTaskGetRunTimeStats` (enable `configGENERATE_RUN_TIME_STATS = 1` and use DWT as the timer source).

**Checkpoint D:** Filled comparison table with measured values.

---

## Deliverables

| # | Item |
|---|------|
| 1 | Part A UART — both core startup messages |
| 2 | Part B2 UART — MU latency measurement (cycles + nanoseconds) |
| 3 | Part C UART — 10 seconds of CM33_1 processing with zero missed samples |
| 4 | Linker script excerpt showing `.sram4` section |
| 5 | Part D comparison table |
| 6 | Written answers to Questions A1, B1, C1, C2 |
| 7 | `shared_mem.h` with ring buffer struct |

---

## Key Concepts Demonstrated

| Concept | Where you saw it |
|---------|-----------------|
| AMP dual-core boot via SYSCON | Part A |
| MU direct 32-bit messaging | Part B |
| IPC latency measurement with DWT | Part B |
| Shared SRAM ring buffer | Part C |
| DMB memory barriers | Part C |
| Throughput vs latency trade-off | Part D |

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| NXP MCXN236 Reference Manual | Chapter MU (Messaging Unit), Chapter SYSCON |
| NXP MCUXpresso SDK | `fsl_mu.h`, multicore examples |
| ARM Cortex-M33 TRM | §A3.4 Memory ordering |
| Barry, *Mastering the FreeRTOS Kernel* | `xTaskNotifyGiveFromISR`, `vTaskGetRunTimeStats` |
