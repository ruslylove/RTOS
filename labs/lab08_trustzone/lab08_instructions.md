# Lab 08 — TrustZone-M: Secure/Non-Secure Partition

**Course:** M.Eng. Real-Time Operating Systems · KMUTNB  
**Platform:** NXP FRDM-MCXN236 (Cortex-M33)  
**Estimated time:** 4–5 hours  
**Builds on:** Lab 07 (memory analysis, static allocation)

---

## Objectives

By completing this lab you will be able to:

1. **Configure** the SAU to partition flash and SRAM into Secure, NSC, and Non-Secure regions.
2. **Implement** a Secure API using the CMSE `__attribute__((cmse_nonsecure_entry))` annotation.
3. **Call** a Secure function from a Non-Secure FreeRTOS task via an NSC veneer.
4. **Trigger** a SecureFault by attempting a direct NS→Secure SRAM access.
5. **Measure** NSC call overhead vs a direct function call.

---

## Prerequisites

- [ ] Lab 07 complete.
- [ ] ARM GNU Toolchain with CMSE support (`arm-none-eabi-gcc` ≥ 10, compiled with `-mcmse`).
- [ ] MCUXpresso IDE or command-line build with separate Secure and Non-Secure projects.
- [ ] SEGGER J-Link or MCU-Link (J-Link firmware) for debugging SecureFault.

---

## Background

### TrustZone-M execution model

The Cortex-M33 has two security states: **Secure (S)** and **Non-Secure (NS)**. The processor is always in one state. Transitions happen only through specific instructions:

- **NS → S:** only through the SG (Secure Gateway) instruction at an address in an NSC region.
- **S → NS:** via `BXNS` / `BLXNS` instructions.

### Two-project build structure

TrustZone-M requires two separate firmware images:

```
Secure project (linked to 0x0000_0000)
  - Runs first after reset
  - Configures SAU, TrustZone peripherals
  - Implements secure API functions
  - Releases Non-Secure image

Non-Secure project (linked to 0x0004_0000)
  - FreeRTOS scheduler
  - Application tasks
  - Calls Secure API through NSC veneers
```

---

## Project Structure

```
lab08_trustzone/
├── secure/                        ← Secure-world project
│   ├── Makefile
│   ├── linker_secure.ld           ← Secure flash + SRAM regions
│   ├── src/
│   │   ├── main_secure.c          ← SAU config, Secure init, NS release
│   │   ├── secure_api.c/.h        ← NSC entry functions
│   │   ├── secure_api_veneer.h    ← auto-generated veneer declarations
│   │   └── startup_MCXN236_s.s    ← Secure vector table
│   └── cmse_implib.o              ← output: NSC import library
├── nonsecure/                     ← Non-Secure FreeRTOS project
│   ├── Makefile
│   ├── linker_nonsecure.ld        ← NS flash + SRAM regions
│   ├── src/
│   │   ├── main_ns.c              ← FreeRTOS init, task creation
│   │   ├── ns_tasks.c/.h          ← tasks that call Secure API
│   │   ├── FreeRTOSConfig.h
│   │   └── uart.h / uart.c
│   └── FreeRTOS-Kernel/
└── flash_both.sh                  ← flashes S image then NS image
```

---

## Part A — SAU Configuration

### Step 1 — Define the memory partition

Edit `secure/src/main_secure.c`:

```c
void SAU_Configure(void)
{
    /* Disable SAU before configuration */
    SAU->CTRL = 0;

    /* Region 0: Non-Secure flash [0x0004_0000 .. 0x001F_FFFF] */
    SAU->RNR  = 0;
    SAU->RBAR = 0x00040000U;
    SAU->RLAR = (0x001FFFFFU & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

    /* Region 1: NSC region in flash [0x0003_FE00 .. 0x0003_FFFF] (512 B) */
    SAU->RNR  = 1;
    SAU->RBAR = 0x0003FE00U;
    SAU->RLAR = (0x0003FFFFU & SAU_RLAR_LADDR_Msk)
              | SAU_RLAR_NSC_Msk | SAU_RLAR_ENABLE_Msk;

    /* Region 2: Non-Secure SRAM [0x2002_0000 .. 0x200F_FFFF] */
    SAU->RNR  = 2;
    SAU->RBAR = 0x20020000U;
    SAU->RLAR = (0x200FFFFFU & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

    /* Enable SAU */
    SAU->CTRL = SAU_CTRL_ENABLE_Msk;
    __DSB();
    __ISB();
}
```

### Step 2 — Verify with `TZM_IsSecure`

Add a diagnostic function that probes whether an address is Secure:

```c
/* Secure-side only — NS code cannot call this */
bool TZM_IsSecure(uint32_t addr)
{
    /* Read SAU to check if addr falls in a Secure (non-NSC, non-NS) region */
    for (int i = 0; i < 8; i++) {
        SAU->RNR = i;
        if (!(SAU->RLAR & SAU_RLAR_ENABLE_Msk)) continue;
        uint32_t base  = SAU->RBAR & SAU_RBAR_BADDR_Msk;
        uint32_t limit = (SAU->RLAR & SAU_RLAR_LADDR_Msk) | 0x1F;
        if (addr >= base && addr <= limit) return false; /* NS or NSC */
    }
    return true; /* not in any NS/NSC region → Secure */
}
```

Call from `main_secure.c` before releasing the NS core:

```c
uart_printf("[S] 0x00010000 is %s\r\n",
    TZM_IsSecure(0x00010000) ? "SECURE" : "NS/NSC");
uart_printf("[S] 0x00050000 is %s\r\n",
    TZM_IsSecure(0x00050000) ? "SECURE" : "NS/NSC");
uart_printf("[S] 0x0003FE00 is %s (NSC)\r\n",
    TZM_IsSecure(0x0003FE00) ? "SECURE" : "NS/NSC");
```

**Checkpoint A:** UART showing correct Secure/NS attribution for three test addresses.

---

## Part B — Implement and Call Secure API

### Step 1 — Write the Secure API

In `secure/src/secure_api.c`:

```c
#include <arm_cmse.h>
#include "secure_api.h"

/* Secure ADC value — NS world must not read this directly */
static volatile int32_t s_last_adc_value = 0;

/* NSC entry function — callable from Non-Secure world */
__attribute__((cmse_nonsecure_entry))
int32_t Secure_ADC_Read(void)
{
    /* Simulate ADC read in Secure world */
    s_last_adc_value = (s_last_adc_value + 37) % 4096;
    return s_last_adc_value;
}

__attribute__((cmse_nonsecure_entry))
void Secure_ADC_Init(void)
{
    uart_printf("[S] Secure_ADC_Init called from NS world\r\n");
    s_last_adc_value = 0;
}
```

In `secure/src/secure_api.h`:

```c
#ifndef SECURE_API_H
#define SECURE_API_H
#include <stdint.h>

/* Declarations for NS world — placed in CMSE import library */
int32_t Secure_ADC_Read(void);
void    Secure_ADC_Init(void);

#endif
```

### Step 2 — Build the Secure image and generate the import library

```bash
cd secure
make
# Output: secure.elf  +  cmse_implib.o (NSC veneer import library)
```

The linker flag `-Wl,--cmse-implib -Wl,--out-implib=cmse_implib.o` generates the import library containing SG stubs.

Verify SG stubs appear in the map file:

```bash
grep "\.gnu\.sgstubs" secure.map
```

Expected:
```
.gnu.sgstubs   0x0003fe00   0x10   Secure_ADC_Read
.gnu.sgstubs   0x0003fe10   0x08   Secure_ADC_Init
```

**Checkpoint B1:** Map file excerpt showing SG stubs in the NSC region.

### Step 3 — Call from Non-Secure FreeRTOS task

Copy `cmse_implib.o` to `nonsecure/`. In `nonsecure/src/ns_tasks.c`:

```c
/* Include Secure API header — functions have NS-safe declarations */
#include "secure_api.h"

void vSensorTask(void *pv)
{
    Secure_ADC_Init();
    for (;;) {
        int32_t value = Secure_ADC_Read();
        uart_printf("[NS] ADC value = %ld\r\n", value);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```

Build the NS image (links against `cmse_implib.o`):

```bash
cd nonsecure
make
```

### Step 4 — Flash and run

```bash
./flash_both.sh
# Flashes secure.bin to 0x0000_0000, then nonsecure.bin to 0x0004_0000
```

Expected output:
```
[S] SAU configured
[S] 0x00010000 is SECURE
[S] 0x00050000 is NS/NSC
[S] Releasing Non-Secure image at 0x00040000
[NS] starting FreeRTOS...
[S] Secure_ADC_Init called from NS world
[NS] ADC value = 37
[NS] ADC value = 74
...
```

**Checkpoint B2:** UART showing alternating `[S]` init and `[NS]` sensor values.

---

## Part C — Trigger a SecureFault

### Step 1 — Direct NS→Secure SRAM access

In `vSensorTask`, add a deliberate access to Secure SRAM (this will fault):

```c
/* Attempting to read Secure SRAM from NS world — should SecureFault */
volatile uint32_t *secure_ptr = (volatile uint32_t *)0x20010000U; /* Secure SRAM */
uint32_t val = *secure_ptr;   /* ← triggers SecureFault */
uart_printf("[NS] should never print: %lu\r\n", val);
```

### Step 2 — Implement SecureFault handler

In `nonsecure/src/startup_MCXN236_ns.s` (or `main_ns.c`):

```c
void SecureFault_Handler(void)
{
    uart_printf("[NS] !!! SecureFault !!!\r\n");
    uart_printf("[NS] SFSR = 0x%08lX\r\n", SAU->SFSR);
    uart_printf("[NS] SFAR = 0x%08lX\r\n", SAU->SFAR);
    for (;;) {}
}
```

**Checkpoint C:** UART showing `!!! SecureFault !!!` with SFSR/SFAR values. Note the faulting address (0x20010000) in SFAR.

> **Question C1:** The SecureFault is raised on the **NS** side (not the Secure side). Explain why — which hardware unit detects the violation and in which security state does the fault handler run?

> **Question C2:** The SFSR (Secure Fault Status Register) has several bits. Look up the meaning of bit 1 (`SFARVALID`) and bit 0 (`INVEP`). Which bit is set in your fault? What does it indicate?

---

## Part D — Measure NSC Call Overhead

### Step 1 — Benchmark NSC vs direct call

Use the DWT cycle counter (Lab 06) to compare:

```c
void vBenchmarkTask(void *pv)
{
    DWT_Enable();
    uint32_t nsc_cycles, direct_cycles;

    /* NSC call overhead */
    DWT_START();
    volatile int32_t v1 = Secure_ADC_Read();   /* crosses S/NS boundary */
    DWT_STOP();
    nsc_cycles = _dwt_elapsed;

    /* Direct function call (replace with a dummy NS function of equal complexity) */
    DWT_START();
    volatile int32_t v2 = ns_dummy_adc_read(); /* NS-only, same logic */
    DWT_STOP();
    direct_cycles = _dwt_elapsed;

    uart_printf("[BENCH] NSC call: %lu cycles  Direct: %lu cycles  Overhead: %lu cycles\r\n",
                nsc_cycles, direct_cycles, nsc_cycles - direct_cycles);
    vTaskDelete(NULL);
}
```

**Checkpoint D:** Terminal showing NSC vs direct cycle counts.

> **Question D1:** The NSC call overhead includes BLXNS instruction + SG instruction + context save. At 150 MHz, convert the overhead cycles to nanoseconds. Is this acceptable for a 1 kHz control loop?

> **Question D2:** FreeRTOS-M33 (NS kernel) allocates a "Secure context" per NS task that calls NSC functions. Where is this context stored, and why is it needed?

---

## Deliverables

| # | Item |
|---|------|
| 1 | Part A UART — SAU probe output (3 addresses) |
| 2 | Part B1 — map file excerpt showing SG stubs in NSC region |
| 3 | Part B2 UART — alternating `[S]` init and `[NS]` sensor values |
| 4 | Part C UART — SecureFault with SFSR/SFAR values |
| 5 | Part D UART — NSC vs direct call cycle counts |
| 6 | Written answers to Questions C1–C2, D1–D2 |
| 7 | `secure_api.c` and `ns_tasks.c` source listings |

---

## Key Concepts Demonstrated

| Concept | Where you saw it |
|---------|-----------------|
| SAU region configuration | Part A |
| Security attribution verification | Part A TZM_IsSecure |
| CMSE `cmse_nonsecure_entry` + SG veneer | Part B |
| NSC import library and BLXNS call | Part B |
| SecureFault on illegal NS→S access | Part C |
| SFSR / SFAR fault diagnosis | Part C |
| NSC call overhead measurement | Part D |

---

## Reference

| Resource | Relevant sections |
|----------|-------------------|
| ARM Cortex-M33 TRM | §B9 TrustZone-M, §B2.3 SecureFault |
| NXP AN13814 | TrustZone-M on MCXN236 |
| ARM CMSE specification | `cmse_nonsecure_entry`, `BLXNS` |
| GCC ARM options | `-mcmse`, `--cmse-implib`, `--out-implib` |
