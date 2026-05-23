---
theme: seriph
background: https://cover.sli.dev
title: "RTOS · Week 15 — Research Frontiers: Mixed-Criticality & Safety"
info: |
  ## Real-Time Operating Systems
  Week 15 — Research Frontiers: Mixed-Criticality & Safety

  KMUTNB · Faculty of Engineering · Department of Electrical and Computer Engineering
  Master of Engineering (M.Eng.) — Electrical and Computer Engineering
class: text-center
transition: slide-left
mdc: true
routeAlias: week-15
fonts:
  sans: Inter
  serif: Lora
  mono: Fira Code
---

# Real-Time Operating Systems

## Week 15 — Research Frontiers

Mixed-Criticality Systems · Safety Standards · RT Linux · Open Research Problems

<div class="pt-10 opacity-70 text-sm">
  KMUTNB · Faculty of Engineering · M.Eng. in Electrical & Computer Engineering
</div>

<div class="abs-br m-6 text-xs opacity-50">
  Reading: Vestal (2007) · IEC 61508 overview · Burns &amp; Davis (2017) survey
</div>

---
layout: two-cols
layoutClass: gap-8
---

# From Week 14 to Week 15

We have covered the full stack: scheduling theory, FreeRTOS internals, security, multi-core, power, and alternative RTOSs.

Week 15 zooms out to **where the field is going** — the open research problems that practicing engineers will encounter.

Mixed-criticality, safety certification, and real-time Linux are shaping the next generation of embedded systems.

::right::

<div class="mt-10 px-5 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-sm leading-relaxed">

**This week — the frontier:**

- Mixed-Criticality Systems (MCS): Vestal model
- Safety standards: IEC 61508, ISO 26262, DO-178C
- RT Linux: PREEMPT_RT patch, latency characteristics
- Multiprocessor real-time scheduling: open problems
- Reading group: Burns & Davis (2017) mixed-criticality survey

</div>

---

# Week 15 — Learning Objectives

By the end of this lecture you will be able to:

<v-clicks>

- **Describe** the Vestal mixed-criticality model and explain the scheduling problem it creates.
- **Distinguish** IEC 61508 SIL levels, ISO 26262 ASIL levels, and DO-178C DAL levels.
- **Explain** what PREEMPT_RT does to the Linux kernel and its current latency characteristics.
- **Identify** three open research problems in multiprocessor real-time scheduling.
- **Critically read** a real-time systems research paper.

</v-clicks>

<div v-click class="mt-6 px-4 py-2 border-l-4 border-amber-500 bg-amber-50 dark:bg-amber-900/20 text-sm">
Maps to <b>CLO 1 &amp; CLO 5</b> — apply scheduling theory and evaluate advanced platform concerns.
</div>

---
layout: section
---

# Part 1
## Mixed-Criticality Systems

---
layout: statement
---

# The Mixed-Criticality Problem

Modern platforms run tasks of different **criticality levels** on the same hardware — a certified safety function alongside an uncertified infotainment app.

<div class="mt-8 text-base opacity-80 max-w-2xl mx-auto">
Classical scheduling assumes WCET estimates are correct. Mixed-criticality theory asks: what should the scheduler do when a low-assurance WCET estimate is violated at runtime? How do we protect high-criticality tasks without over-provisioning?
</div>

---

# Vestal Model (2007)

Steve Vestal's seminal paper introduced the formal MCS model:

<div class="mt-4 text-sm">

| Parameter | Meaning |
|-----------|---------|
| Criticality level χ(τᵢ) | HI (high) or LO (low) — maps to certification level |
| $C_i^{LO}$ | WCET claimed at LO mode (optimistic, uncertified estimate) |
| $C_i^{HI}$ | WCET claimed at HI mode (certified, conservative estimate, $C_i^{HI} \ge C_i^{LO}$) |
| System mode | LO mode (normal) or HI mode (triggered by LO task overrun) |

</div>

**Mode switch rule:** if any task executes beyond its $C_i^{LO}$ bound, the system switches to HI mode. In HI mode, LO-criticality tasks are **dropped** (not scheduled).

<div v-click class="mt-4 text-sm px-4 py-2 border-l-4 border-amber-500 bg-amber-50 dark:bg-amber-900/20">
The key insight: in LO mode, HI tasks run with $C_i^{LO}$ (conserving bandwidth for LO tasks). In HI mode, HI tasks run with $C_i^{HI}$ (full certification budget). This dual-budget approach avoids over-provisioning in normal operation.
</div>

---

# MCS Scheduling Algorithms

<div class="mt-4 text-sm">

| Algorithm | Key idea | Status |
|-----------|---------|--------|
| **EDF-VD** (Vestal/Baruah 2011) | Virtual deadlines: HI tasks get earlier virtual deadline in LO mode | Optimal for 2-level MC under EDF |
| **AMC** (Burns & Baruah 2011) | Audsley's Multiprocessor Criticality — RMS-based | Widely used in practice |
| **MC²** (Herman et al.) | Two-level hierarchical scheduling for real MC architectures | Research prototype |
| **FMTV** challenge | Industry MCS benchmarks — automotive, avionics | Ongoing (ECRTS workshop) |

</div>

<div v-click class="mt-4 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
Current gap: most MCS theory assumes uniprocessor. Multiprocessor MCS scheduling with mixed-criticality task migration is still an active research area. FreeRTOS and Zephyr do not implement MCS algorithms natively — a research opportunity for M.Eng. theses.
</div>

---
layout: section
---

# Part 2
## Safety Standards

---

# IEC 61508 — Functional Safety

IEC 61508 defines **Safety Integrity Levels (SIL)** for E/E/PE safety-related systems:

<div class="mt-3 text-sm">

| SIL | Probability of dangerous failure per hour | Example |
|-----|------------------------------------------|---------|
| SIL 1 | 10⁻⁵ – 10⁻⁶ | Industrial conveyor safety stop |
| SIL 2 | 10⁻⁶ – 10⁻⁷ | Emergency shutdown in process plant |
| SIL 3 | 10⁻⁷ – 10⁻⁸ | Railway signalling, medical device |
| SIL 4 | 10⁻⁸ – 10⁻⁹ | Nuclear reactor protection |

</div>

**RTOS implications for SIL 2+:**

- Static memory allocation mandatory (heap_1 or static allocation)
- Stack bounds statically verified
- No dynamic task creation after init
- WCET verified on target hardware, not estimated
- Certified compiler (GCC CERT, IAR, Keil with certification pack)

---

# ISO 26262 and DO-178C

<div class="grid grid-cols-2 gap-6 mt-4 text-sm">

<div class="px-4 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30">

### ISO 26262 — Automotive

ASIL A–D (D = most critical).

**ASIL D examples:** electronic power steering, ABS, airbag control.

FreeRTOS-MCU Plus (TI/NXP) has ASIL-B qualification support. Full ASIL-D requires a qualified RTOS kernel + qualified compiler + tool qualification evidence.

**Key additions vs SIL:**
- Fault Tree Analysis (FTA)
- FMEA (Failure Mode & Effects Analysis)
- Random hardware fault coverage requirements

</div>

<div class="px-4 py-4 rounded-lg bg-green-50 dark:bg-green-900/30">

### DO-178C — Avionics Software

DAL A–E (A = most critical, e.g. flight control).

**DAL A:** every line of code must have structural coverage (MC/DC). Every requirement must be tested. Formal verification for critical modules.

**RTOS for DAL A/B:**
- INTEGRITY RTOS (Green Hills)
- LynxOS-178 (Lynx Software)
- VxWorks 653 (Wind River)

FreeRTOS is used up to DAL C/D with qualification kits. DO-178C DAL A with FreeRTOS requires significant qualification effort.

</div>

</div>

---
layout: section
---

# Part 3
## Real-Time Linux

---
layout: two-cols
layoutClass: gap-6
---

# PREEMPT_RT Patch

Linux is not a real-time OS by default: kernel locks, non-preemptible sections, and interrupt latency spikes in the millisecond range.

**PREEMPT_RT** (now mainline since Linux 6.12) converts:

- Spinlocks → sleeping mutexes (preemptible)
- Interrupt handlers → threaded IRQs (preemptible)
- Timer wheel → high-resolution timers
- All kernel paths → fully preemptible

**Current latency (measured on x86/ARM):**

| System | Worst-case latency |
|--------|--------------------|
| Standard Linux | 1–100 ms |
| PREEMPT_RT | 50–200 µs |
| Xenomai (dual-kernel) | < 10 µs |
| Bare-metal RTOS (Cortex-M33) | < 2 µs |

::right::

<div class="mt-6 px-5 py-4 rounded-lg bg-amber-50 dark:bg-amber-900/30 text-sm leading-relaxed">

### When to use RT Linux

PREEMPT_RT is ideal for:

- Industrial PCs (robot controllers, CNC machines)
- Soft real-time with tight deadlines (audio, video sync)
- Systems where Linux ecosystem (networking, containers, ROS2) is required alongside RT

Not suitable for:

- Hard real-time with µs-level deadlines
- Safety certification (no SIL/ASIL path for standard Linux)
- Ultra-low-power embedded (Linux overhead too high)

<div class="mt-3 text-xs opacity-70">
ROS2 Humble uses PREEMPT_RT Linux as its recommended real-time platform. Many industrial robot controllers run RT Linux + a small RTOS co-processor for the inner control loop.
</div>

</div>

---
layout: section
---

# Part 4
## Open Research Problems

---

# Where the Field is Moving

<div class="mt-4 text-sm">

| Problem | Why it is hard | Current state |
|---------|---------------|---------------|
| **Multiprocessor MCS** | Task migration + mode switch invalidates per-task analysis | Open — ECRTS, RTSS papers |
| **Neural network inference latency** | Non-deterministic data-dependent WCET | Active — WCET for NNs on NPUs |
| **RTOS for ML pipelines** | Soft real-time with variable deadlines; sensor fusion | Emerging — TensorFlow Lite RTOS |
| **Formal verification of RTOS kernels** | seL4 proved for C kernel; FreeRTOS informal | seL4 done; FreeRTOS partial (CBMC) |
| **Energy-aware scheduling** | Combine EDF/RMS with DVFS; optimality under energy constraints | Ongoing |
| **TrustZone-M for MCS** | Use Secure world to enforce criticality isolation | Research prototype stage |

</div>

<div v-click class="mt-3 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
M.Eng. thesis opportunities: multiprocessor MCS on MCXN236, TrustZone-M as a criticality isolation mechanism, WCET analysis for ML inference on Cortex-M55 with Helium.
</div>

---
layout: section
---

# Part 5
## Paper Reading Group

---

# Burns & Davis (2017) — MCS Survey

**"A Survey of Research into Mixed Criticality Systems"** — ACM Computing Surveys, 2017.

<div class="mt-4 text-sm">

Key sections to read for this week:

| Section | Topic |
|---------|-------|
| §2 | Motivating examples — avionics IMA, automotive E/E |
| §3 | Vestal model formal definition |
| §4 | Uniprocessor MCS algorithms (EDF-VD, AMC-rtb) |
| §5 | Multiprocessor MCS — why it is harder |
| §7 | Open problems |

</div>

<div class="mt-4 text-sm px-4 py-2 border-l-4 border-amber-500 bg-amber-50 dark:bg-amber-900/20">

**Discussion questions for next class:**

1. What is the fundamental scheduling assumption that the Vestal model relaxes?
2. EDF-VD is optimal for 2-criticality-level uniprocessor. Does this result extend to 3 levels? Why or why not?
3. The survey lists "response time analysis for MCS" as open. What makes the existing RTA from Week 3 insufficient for the mixed-criticality case?

</div>

---
layout: default
---

# Key Takeaways

<v-clicks>

- **Mixed-Criticality Systems** run tasks with different safety certification levels on shared hardware. The Vestal model uses dual WCET budgets ($C^{LO}$/$C^{HI}$) and mode switching to balance certification cost against utilisation.
- **Safety standards** (IEC 61508, ISO 26262, DO-178C) impose strict RTOS requirements at higher integrity levels: static allocation, certified compilers, formal verification. FreeRTOS has qualification kits for mid-level assurance.
- **PREEMPT_RT** Linux (mainlined in v6.12) achieves 50–200 µs worst-case latency — suitable for soft RT and industrial robotics, but not hard RT or safety certification.
- **Open problems**: multiprocessor MCS scheduling, WCET for neural networks, TrustZone-M as a criticality barrier — active research areas with direct relevance to future embedded designs.

</v-clicks>

<div v-click class="mt-5 text-center text-base px-4 py-2 rounded bg-blue-100 dark:bg-blue-900/40">
Next — <b>Week 16: Final Project Presentations</b> — present your RTOS design project.
</div>

---

# Before Next Week (Final Project)

<div class="grid grid-cols-2 gap-8 mt-6">

<div>

### Reading
- **Burns & Davis (2017)** — "A Survey of Research into Mixed Criticality Systems" (ACM Computing Surveys)
- **Vestal (2007)** — "Preemptive Scheduling of Multi-Criticality Systems with Varying Degrees of Execution Time Assurance"
- Review IEC 61508 Part 3 summary (freely available online)

### Preparation
- **Final project**: complete implementation and write-up
- Prepare 15-minute presentation + 5-minute Q&A
- Upload report (PDF) to course LMS by Week 16 morning

</div>

<div>

### Check yourself
<div class="text-sm">

1. A task set has $C_i^{LO} = 2$ ms and $C_i^{HI} = 5$ ms. In LO mode, which budget does the scheduler use?
2. An ISO 26262 ASIL-D airbag system uses dynamic heap allocation. Is this acceptable? Why or why not?
3. PREEMPT_RT achieves 200 µs worst-case latency. A motor controller needs 50 µs deadline. Which RTOS strategy should you use?
4. Name one open problem in multiprocessor MCS and sketch why existing uniprocessor theory does not apply.

</div>

</div>

</div>

---
layout: end
class: text-center
---

# Week 15 Complete

Mixed-Criticality & Safety Standards

<div class="mt-4 text-sm opacity-70">
Real-Time Operating Systems · KMUTNB · M.Eng. ECE<br/>
Next — Week 16 · Final Project Presentations
</div>

<style>
:root { --slidev-theme-primary: #003874; }
.slidev-layout h1 { color: #003874; }
.dark .slidev-layout h1 { color: #7ba7d9; }
table { font-size: 0.92em; }
</style>
