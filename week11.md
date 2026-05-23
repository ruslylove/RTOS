---
theme: seriph
background: https://cover.sli.dev
title: "RTOS · Week 11 — ARM TrustZone-M Security"
info: |
  ## Real-Time Operating Systems
  Week 11 — ARM TrustZone-M Security

  KMUTNB · Faculty of Engineering · Department of Electrical and Computer Engineering
  Master of Engineering (M.Eng.) — Electrical and Computer Engineering
class: text-center
transition: slide-left
mdc: true
routeAlias: week-11
fonts:
  sans: Inter
  serif: Lora
  mono: Fira Code
---

# Real-Time Operating Systems

## Week 11 — ARM TrustZone-M Security

SAU · IDAU · NSC regions · Secure/Non-Secure partitioning

<div class="pt-10 opacity-70 text-sm">
  KMUTNB · Faculty of Engineering · M.Eng. in Electrical & Computer Engineering
</div>

<div class="abs-br m-6 text-xs opacity-50">
  Reading: ARM Cortex-M33 TRM §B9 · NXP AN13814
</div>

---
layout: two-cols
layoutClass: gap-8
---

# From Week 10 to Week 11

We can now manage and protect memory with the MPU.

But the MPU only controls access permissions — it cannot prevent a compromised task from escalating to privileged mode.

TrustZone-M provides **hardware-enforced security domains** that survive even a fully compromised Non-Secure world.

::right::

<div class="mt-10 px-5 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-sm leading-relaxed">

**This week — hardware security partitioning:**

- TrustZone-M architecture overview
- Security Attribution Unit (SAU) and IDAU
- Non-Secure Callable (NSC) regions and the SG instruction
- MCXN236 TrustZone configuration
- Secure/Non-Secure FreeRTOS split
- Lab 8: partition a sensor task into Secure/Non-Secure

</div>

---

# Week 11 — Learning Objectives

By the end of this lecture you will be able to:

<v-clicks>

- **Explain** the TrustZone-M security model and the two execution worlds.
- **Describe** the role of the SAU and IDAU in security attribution.
- **Explain** how NSC regions and the SG instruction implement secure entry points.
- **Configure** the SAU on MCXN236 to partition flash and SRAM.
- **Describe** how FreeRTOS-M33 runs Non-Secure tasks under a Secure kernel.

</v-clicks>

<div v-click class="mt-6 px-4 py-2 border-l-4 border-amber-500 bg-amber-50 dark:bg-amber-900/20 text-sm">
Maps to <b>CLO 5</b> — evaluate security mechanisms available in modern RTOS platforms.
</div>

---
layout: section
---

# Part 1
## TrustZone-M Architecture Overview

---
layout: statement
---

# Two Worlds — One Core

TrustZone-M partitions the Cortex-M33 into a **Secure world** and a **Non-Secure world**. The hardware enforces the boundary on every memory access and every branch instruction.

<div class="mt-8 text-base opacity-80 max-w-2xl mx-auto">
A compromised Non-Secure application cannot read Secure memory, cannot call Secure code except through approved NSC entry points, and cannot access Secure peripherals. The boundary is enforced in silicon — no software patch can bypass it.
</div>

---

# TrustZone-M Key Concepts

<div class="mt-4 text-sm">

| Concept | Meaning |
|---------|---------|
| **Secure world (S)** | Trusted code — bootloader, crypto keys, attestation, secure storage |
| **Non-Secure world (NS)** | Application code — FreeRTOS tasks, drivers, user code |
| **SAU** | Security Attribution Unit — CPU-side table of region attributes |
| **IDAU** | Implementation-Defined Attribution Unit — SoC-level fixed mapping (higher priority than SAU) |
| **NSC region** | Non-Secure Callable flash — the only way NS code enters Secure world |
| **SG instruction** | Secure Gateway — the one permitted entry point into Secure code |
| **CMSE** | ARM C extension for TrustZone-M (`__attribute__((cmse_nonsecure_entry))`) |

</div>

<div v-click class="mt-4 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
Rule: Secure code can call Non-Secure code (via function pointer + <code>BLXNS</code>). Non-Secure code can only enter Secure code through an SG instruction in an NSC region.
</div>

---
layout: section
---

# Part 2
## SAU and IDAU

---
layout: two-cols
layoutClass: gap-6
---

# SAU — Security Attribution Unit

The SAU is a CPU register-based table (up to 8 regions on Cortex-M33). Each entry defines:

- Base address
- Limit address
- Type: **Secure**, **Non-Secure**, or **NSC** (Non-Secure Callable)

```c
/* Configure SAU region 0: Non-Secure flash */
SAU->RNR  = 0;
SAU->RBAR = 0x00200000U;          /* NS flash base */
SAU->RLAR = 0x003FFFCAU           /* limit */
          | SAU_RLAR_ENABLE_Msk;  /* enable region */

/* Configure SAU region 1: NSC region */
SAU->RNR  = 1;
SAU->RBAR = 0x0000FE00U;          /* NSC base */
SAU->RLAR = 0x0000FFCAU
          | SAU_RLAR_NSC_Msk      /* NSC flag */
          | SAU_RLAR_ENABLE_Msk;

/* Enable SAU */
SAU->CTRL = SAU_CTRL_ENABLE_Msk;
```

::right::

<div class="mt-6 px-5 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-sm leading-relaxed">

### IDAU — Implementation-Defined

On MCXN236, the IDAU maps:

| Address range | Attribution |
|--------------|-------------|
| 0x0000_0000 – 0x000F_FFFF | Secure alias (Flash) |
| 0x0010_0000 – 0x001F_FFFF | NSC alias |
| 0x0020_0000 – 0x003F_FFFF | Non-Secure Flash |
| 0x1000_0000 – 0x1FFF_FFFF | Secure SRAM |
| 0x2000_0000 – 0x2FFF_FFFF | Non-Secure SRAM |

<div class="mt-3 text-xs opacity-70">
IDAU takes precedence over SAU when IDAU marks a region Secure. SAU can only add NS/NSC regions within IDAU-NS ranges.
</div>

</div>

---
layout: section
---

# Part 3
## NSC Regions and the SG Instruction

---

# Crossing the Security Boundary

Non-Secure code cannot branch directly into Secure flash — the hardware raises a SecureFault. The only legal crossing uses a **Secure Gateway (SG)** instruction located in an **NSC region**.

```c
/* Secure side: define an NSC entry point */
/* Place in NSC region via linker section attribute */
__attribute__((cmse_nonsecure_entry))
__attribute__((section(".gnu.sgstubs")))
int32_t Secure_GetSensorValue(void)
{
    /* This code runs in Secure world */
    return secure_sensor_read();
}
```

```c
/* Non-Secure side: call through the veneer */
/* Compiler generates BLXNS to the SG stub */
typedef int32_t (*NS_FuncPtr_t)(void) __attribute__((cmse_nonsecure_call));

extern int32_t Secure_GetSensorValue(void);
int32_t val = Secure_GetSensorValue();   /* crosses boundary */
```

<div v-click class="mt-3 text-sm px-4 py-2 border-l-4 border-green-500 bg-green-50 dark:bg-green-900/20">
The linker places an <b>SG + branch</b> veneer in the NSC region. CMSE compiler attribute generates <code>BLXNS</code> on the NS side. The hardware checks: (1) target is NSC, (2) first instruction is SG — if either fails: SecureFault.
</div>

---
layout: section
---

# Part 4
## MCXN236 TrustZone Configuration

---

# MCXN236 TrustZone Partition

<div class="mt-4 text-sm">

| Region | Size | World | Purpose |
|--------|------|-------|---------|
| Flash 0x0000_0000 | 256 KB | Secure | Secure bootloader + keys |
| Flash NSC 0x0003_FE00 | 512 B | NSC | Entry point veneers |
| Flash 0x0004_0000 | 1.75 MB | Non-Secure | Application + FreeRTOS |
| SRAM 0x1000_0000 | 128 KB | Secure | Secure heap + stack |
| SRAM 0x2000_0000 | 384 KB | Non-Secure | NS FreeRTOS heap |
| Peripherals | — | Per-peripheral | Configured via AHBSC |

</div>

```c
/* AHBSC peripheral security: mark UART0 as Non-Secure */
AHBSC0->SLAVE_RULE0 |= AHBSC_SLAVE_RULE0_LPUART0(0x1U); /* NS */

/* Mark DMA0 as Non-Secure */
AHBSC0->SLAVE_RULE0 |= AHBSC_SLAVE_RULE0_DMA0(0x1U);    /* NS */
```

<div v-click class="mt-3 text-sm px-4 py-2 border-l-4 border-amber-500 bg-amber-50 dark:bg-amber-900/20">
The AHBSC (AHB Secure Controller) on MCXN236 lets you assign each peripheral independently to Secure, Non-Secure, or privilege-controlled access. Always check the RM peripheral table before enabling a driver.
</div>

---
layout: section
---

# Part 5
## FreeRTOS with TrustZone-M

---
layout: two-cols
layoutClass: gap-6
---

# Secure/Non-Secure FreeRTOS Split

FreeRTOS-M33 supports two configurations:

**Option A — Secure kernel (Cortex-M33 TFM model):**
- FreeRTOS scheduler runs in Secure world
- All tasks are Secure; NS code called from Secure tasks
- Maximum isolation; significant porting effort

**Option B — Non-Secure kernel (recommended for most products):**
- FreeRTOS scheduler runs in Non-Secure world
- Secure world provides a small Secure Service Layer (SSL)
- NS tasks call Secure APIs via NSC veneers

```c
/* NS side: call Secure API from a FreeRTOS task */
void vSensorTask(void *pv) {
    for (;;) {
        int32_t raw = Secure_GetSensorValue(); /* NSC call */
        publish_reading(raw);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

::right::

<div class="mt-6 px-5 py-4 rounded-lg bg-green-50 dark:bg-green-900/30 text-sm leading-relaxed">

### Secure context allocation

When a Non-Secure task makes an NSC call, TrustZone-M saves NS context and switches to Secure stack. FreeRTOS must allocate a **Secure context** per task that makes NSC calls:

```c
/* In FreeRTOSConfig.h */
#define configENABLE_TRUSTZONE   1
#define secureconfigMAX_SECURE_CONTEXTS  5

/* In task creation */
xTaskCreate(vSensorTask, "Sensor",
            256, NULL, 2, &xHandle);
/* FreeRTOS allocates a Secure context
   for this task automatically */
```

<div class="mt-3 text-xs opacity-70">
Each Secure context costs ~160 bytes of Secure SRAM. Size <code>secureconfigMAX_SECURE_CONTEXTS</code> for the number of tasks that call NSC functions.
</div>

</div>

---
layout: section
---

# Part 6
## Lab 8 — Secure Sensor Partition

---
layout: two-cols
layoutClass: gap-6
---

# Lab 8 — Partition a Sensor Driver

<v-clicks>

**Step 1 — Configure partitioning:**
1. Set up SAU regions: Secure flash [0,256KB), NSC [255.5,256KB), NS flash [256KB,2MB)
2. Verify the partition with `TZM_IsSecure(addr)` probe function

**Step 2 — Implement Secure API:**
3. Write `Secure_ADC_Init()` and `Secure_ADC_Read()` with `__attribute__((cmse_nonsecure_entry))`
4. Place in NSC linker section; confirm SG stubs appear in map file

**Step 3 — Call from NS FreeRTOS task:**
5. Create two NS tasks: vSensorTask (reads via NSC) and vDisplayTask (prints result)
6. Verify NS tasks cannot directly read Secure SRAM (fault expected)

</v-clicks>

::right::

<div class="mt-8 px-5 py-4 rounded-lg bg-amber-50 dark:bg-amber-900/30 text-sm leading-relaxed">

**What to submit**

- SAU configuration code with region table
- Secure API header and implementation
- SystemView trace showing NS task running + NSC entry + return
- Screenshot of SecureFault triggered by direct NS→Secure SRAM access
- Memory map: Secure SRAM usage vs NS heap usage

<div class="mt-3 text-xs opacity-70">
Reading — ARM Cortex-M33 TRM §B9 · NXP AN13814 TrustZone-M on MCXN236
</div>

</div>

---
layout: default
---

# Key Takeaways

<v-clicks>

- TrustZone-M creates two hardware-enforced worlds: **Secure** and **Non-Secure**. A compromised NS application cannot access S memory or peripherals.
- The **SAU** (up to 8 regions) and **IDAU** (SoC-fixed map) together determine security attribution for every address. IDAU takes precedence.
- **NSC regions** contain SG instruction veneers — the only legal NS→S crossing. CMSE compiler attributes generate correct `BLXNS` calls and SG stubs automatically.
- On **MCXN236**, the AHBSC assigns each peripheral to Secure or Non-Secure independently. Always configure peripheral attribution before enabling drivers in NS code.
- FreeRTOS-M33 running in **Non-Secure world** is the recommended split: small Secure Service Layer + full NS RTOS. Each NS task that calls NSC functions needs a Secure context allocation (`~160 B` of Secure SRAM).

</v-clicks>

<div v-click class="mt-5 text-center text-base px-4 py-2 rounded bg-blue-100 dark:bg-blue-900/40">
Next — <b>Week 12: Multi-Core AMP vs SMP</b> — running FreeRTOS on both Cortex-M33 cores of MCXN236.
</div>

---

# Before Next Week

<div class="grid grid-cols-2 gap-8 mt-6">

<div>

### Reading
- **ARM Cortex-M33 TRM**, §B9 — TrustZone-M
- **NXP AN13814** — TrustZone-M configuration on MCXN236
- **ARM CMSE specification** — cmse_nonsecure_entry attribute

### Lab
- Complete **Lab 8** — Secure/Non-Secure partition
- Trigger and capture a SecureFault in the debugger
- Measure NSC call overhead vs direct function call

</div>

<div>

### Check yourself
<div class="text-sm">

1. A Non-Secure task writes to address 0x1000_0100 (Secure SRAM). What happens at the hardware level?
2. Why must the SG instruction be the first instruction in an NSC entry veneer?
3. You have 5 tasks, but only 2 call NSC functions. What is the minimum value of `secureconfigMAX_SECURE_CONTEXTS`?
4. Can Secure code call a Non-Secure function directly (not through NSC)? What instruction is used?

</div>

</div>

</div>

---
layout: end
class: text-center
---

# Week 11 Complete

ARM TrustZone-M Security

<div class="mt-4 text-sm opacity-70">
Real-Time Operating Systems · KMUTNB · M.Eng. ECE<br/>
Next — Week 12 · Multi-Core AMP vs SMP
</div>

<style>
:root { --slidev-theme-primary: #003874; }
.slidev-layout h1 { color: #003874; }
.dark .slidev-layout h1 { color: #7ba7d9; }
table { font-size: 0.92em; }
</style>
