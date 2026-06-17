# Lab 08 — MPU Memory Protection

**Platform:** WeAct STM32H503CBT6 · **Estimated time:** 2 hours

## Objectives
1. Understand the Cortex-M33 **ARM v8-M MPU** register interface.
2. Configure an MPU region to make a data buffer **read-only**.
3. Observe a **MemManage fault** when code attempts an illegal write.
4. Decode the `MMFSR` / `MMFAR` fault status registers to identify the violation.

## ARM v8-M MPU Primer

The STM32H503 Cortex-M33 provides **8 MPU regions** (check `MPU->TYPE[15:8]`).  
Each region is defined by two registers:

| Register | Bits[31:5] | Bits[4:3] | Bits[2:1] | Bit[0] |
|----------|------------|-----------|-----------|--------|
| `RBAR`   | BASE addr  | SH (shareability) | AP (access) | XN (exec-never) |
| `RLAR`   | LIMIT addr | —         | AttrIndx  | EN (enable) |

**AP field (RBAR[2:1]) — privileged access permissions:**

| AP  | Privileged | Unprivileged |
|-----|-----------|--------------|
| 0b00 | R/W       | No access   |
| 0b01 | R/W       | R/W          |
| 0b10 | **R only**  | No access  |
| 0b11 | R only    | R only       |

Since FreeRTOS non-MPU port runs all tasks in **privileged** mode, `AP=0b10` makes
the region read-only even for FreeRTOS tasks.

**MAIR (Memory Attribute Indirection Register):** each byte encodes a memory type.
- `0x44` = Normal, Non-Cacheable (used for RAM in this lab)
- `0x00` = Device-nGnRnE (used for peripheral space)

**Region granularity:** 32 bytes minimum — both BASE and LIMIT must be 32-byte aligned.

## Lab Design

```
secure_buffer[32]  ← __attribute__((aligned(32)))
                     placed in .data section at link time

MPU Region 0 ─────────────────────────────────────────
  Base  = &secure_buffer       (32-byte aligned)
  Limit = &secure_buffer + 31  (one 32-byte granule)
  AP    = 0b10  (Read-Only, privileged)
  XN    = 1     (data — execute-never)
  PRIVDEFENA = 1  (everything else uses default ARM map)

vNormalTask ── reads secure_buffer  ── OK (RO read is allowed)
vRogueTask  ── writes secure_buffer ── MemManage fault!
```

## Expected Output

### Normal mode (`USE_ROGUE_TASK 0`):
```
=== RTOS Lab 08: MPU Memory Protection ===
Cortex-M33 ARM v8-M MPU — 8 regions available

── MPU Configuration ──
  TYPE : 0x00000800  (8 regions supported)
  CTRL : 0x00000005  ENABLE=1  HFNMIENA=0  PRIVDEFENA=1
  Region 0: RBAR=0x200xxxX5  RLAR=0x200xxxX1
    Base=0x200xxxX0  AP=0b10  XN=1  EN=1
    secure_buffer @ 0x200xxxX0  size=32 bytes
────────────────────────

[main] USE_ROGUE_TASK=0 — normal read-only demo mode
[Normal #1] secure_buffer = "SECRET: read-only protected data"
[Normal #2] secure_buffer = "SECRET: read-only protected data"
...
```

### Fault mode (`USE_ROGUE_TASK 1`):
```
[Rogue] Attempting WRITE to protected region 0x200xxxX0 ...

[MPU FAULT] MMFSR=0x82  MMFAR=0x200xxxX0
  DACCVIOL=1  IACCVIOL=0  MMARVALID=1
  => Write attempt at 0x200xxxX0 blocked by MPU Region 0
```

> `MMFSR=0x82`: bit 7 (MMARVALID=1) + bit 1 (DACCVIOL=1)

## Experiments

### Exp 1 — Confirm MPU region settings
With `USE_ROGUE_TASK 0`, run the lab and match the printed `RBAR`/`RLAR` values
against the bit table above.

> **Q1:** What is the 32-byte aligned base address of `secure_buffer` on your board?  
> Verify it matches `secure_buffer @ 0x...` in the dump. Why must it be 32-byte aligned?

### Exp 2 — Trigger and analyse the MemManage fault
Change `USE_ROGUE_TASK` to `1` in `mpu_tasks.h`, rebuild, and flash.

> **Q2:** What are the values of `MMFSR` and `MMFAR`?  
> Decode each set bit in `MMFSR` using the bit table in the primer.  
> Does `MMFAR` match the address of `secure_buffer`?

### Exp 3 — Read-only from multiple tasks
Still with `USE_ROGUE_TASK 0`, create a second normal task at a different priority
that also reads from `secure_buffer`. Confirm both tasks can read concurrently.

> **Q3:** Why are concurrent **reads** to a shared buffer safe without a mutex,
> while concurrent **writes** (even to unprotected memory) are not?

### Exp 4 — Expand protection to multiple regions
Add a second MPU region (Region 1) that marks a 64-byte `audit_log[]` array
as **no-execute only** (XN=1, AP=0b01 for R/W from any privilege).

> **Q4:** How does a **no-execute** region differ in purpose from a **read-only** region?  
> Give a real-world example where you would use each.

## Deliverables
| # | Item |
|---|------|
| 1 | Serial log showing MPU config dump (Exp 1) |
| 2 | Serial log of MemManage fault with decoded `MMFSR`/`MMFAR` (Exp 2) |
| 3 | Written answers to Q1–Q4 |

## Implementation Notes

- `mpu_init()` must be called **before** `vTaskStartScheduler()`.  
  The MPU is active from the moment `MPU->CTRL` is set; no scheduler required.
- `SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk` is essential — without it, MemManage  
  faults escalate to **HardFault**, bypassing `MemManage_Handler`.
- The `__DMB()` before enabling the MPU is a data memory barrier ensuring all region  
  writes are visible before the enable takes effect.
- `PRIVDEFENA=1` keeps flash, RAM, and peripheral access intact for regions not covered  
  by a programmed MPU entry; without it, accessing any uncovered address would fault.
