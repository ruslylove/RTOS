/*
 * shared_mem.h -- Shared SRAM4 data structures (both CM33_0 and CM33_1)
 *
 * This header is included by BOTH sub-projects.  The actual storage is
 * placed in the .sram4 linker section (0x2008_0000) which is accessible
 * by both cores.
 *
 * Key rules:
 *   - Always use __DMB() BEFORE writing write_idx (producer).
 *   - Always use __DMB() AFTER reading write_idx (consumer).
 *   - Never pass a pointer INTO the ring buffer across the MU -- only
 *     pass the index.
 *
 * Lab 09 -- Dual-Core AMP: Messaging Unit & Shared SRAM
 * Platform: NXP FRDM-MCXN236 (Cortex-M33 x2)
 */
#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include <stdint.h>

/* ── Ring buffer (Part C) ────────────────────────────────────────────────── */
#define RING_BUF_SIZE  256U   /* must be a power of two */

/*
 * RingBuf_t -- lock-free single-producer / single-consumer ring buffer.
 *
 * Producer (CM33_0):
 *   1. Check (write_idx + 1) % SIZE != read_idx  (not full)
 *   2. Write data[write_idx]
 *   3. __DMB()                  <- store visible before index update
 *   4. write_idx = next
 *   5. __DMB()                  <- index visible before MU send
 *   6. MU_TrySendMsg(...)       <- notify consumer
 *
 * Consumer (CM33_1):
 *   1. On MU notification: ulTaskNotifyTakeFromISR
 *   2. __DMB()                  <- ensure data visible after MU read
 *   3. Read data[read_idx]
 *   4. read_idx = (read_idx + 1) % SIZE
 */
typedef struct {
    volatile uint32_t write_idx;            /* updated by CM33_0 */
    volatile uint32_t read_idx;             /* updated by CM33_1 */
    volatile uint32_t missed;               /* dropped samples (full buffer) */
    int16_t           data[RING_BUF_SIZE];  /* sample storage */
} __attribute__((packed, aligned(4))) RingBuf_t;

/*
 * g_ring -- the shared ring buffer instance.
 *
 * Placed in the .sram4 linker section in cm33_0's linker script.
 * CM33_1's linker script must map SRAM4 at the same address (read-only
 * from the linker's perspective -- no ORIGIN conflict).
 */
extern RingBuf_t g_ring;

/* ── IPC latency measurement (Part B) ───────────────────────────────────── */
/*
 * LatencyTest_t -- timestamp pair for measuring MU round-trip latency.
 *
 * Usage:
 *   CM33_0: g_latency.send_cycles = DWT->CYCCNT; __DMB(); MU_SendMsg(...)
 *   CM33_1: g_latency.recv_cycles = DWT->CYCCNT; __DMB();  (first ISR lines)
 *   CM33_1: latency = recv_cycles - send_cycles;
 */
typedef struct {
    volatile uint32_t send_cycles;  /* written by CM33_0 just before MU send */
    volatile uint32_t recv_cycles;  /* written by CM33_1 at ISR entry */
} __attribute__((packed)) LatencyTest_t;

extern LatencyTest_t g_latency;

#endif /* SHARED_MEM_H */
