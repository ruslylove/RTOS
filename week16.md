---
theme: seriph
background: https://cover.sli.dev
title: "RTOS · Week 16 — Final Project Presentations"
info: |
  ## Real-Time Operating Systems
  Week 16 — Final Project Presentations

  KMUTNB · Faculty of Engineering · Department of Electrical and Computer Engineering
  Master of Engineering (M.Eng.) — Electrical and Computer Engineering
class: text-center
transition: slide-left
mdc: true
routeAlias: week-16
fonts:
  sans: Inter
  serif: Lora
  mono: Fira Code
---

# Real-Time Operating Systems

## Week 16 — Final Project Presentations

Project showcase · Peer review · Course close

<div class="pt-10 opacity-70 text-sm">
  KMUTNB · Faculty of Engineering · M.Eng. in Electrical & Computer Engineering
</div>

<div class="abs-br m-6 text-xs opacity-50">
  Presentations: 15 min + 5 min Q&amp;A per team
</div>

---
layout: two-cols
layoutClass: gap-8
---

# Week 16 Overview

This is the culmination of 15 weeks of RTOS theory and practice.

Each team presents their project — a complete FreeRTOS (or Zephyr) application on the FRDM-MCXN236, integrating at least three concepts from the course.

Feedback is given in real time by peers and the instructor.

::right::

<div class="mt-10 px-5 py-4 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-sm leading-relaxed">

**Today's agenda:**

- 09:00 — Project presentations (15 min + 5 min Q&A per team)
- Peer review forms submitted after each presentation
- Final report due: uploaded to LMS before class starts
- Course wrap-up and CLO mapping review
- Open Q&A

</div>

---
layout: section
---

# Final Project

## Requirements & Deliverables

---

# Project Requirements

Your project must demonstrate mastery of **at least three** of the following course topics:

<div class="mt-4 text-sm">

<v-clicks>

- **Scheduling**: justify task priorities and periods using RMS or EDF analysis; verify schedulability analytically
- **IPC**: use at least two FreeRTOS primitives (queue, semaphore, mutex, event group) correctly
- **Synchronisation**: handle a shared resource safely — no race conditions; document the critical sections
- **Memory**: correct stack sizing using high-water-mark; use heap_4 or static allocation appropriately
- **WCET / timing**: measure at least one task's execution time with DWT; verify it meets its budget
- **Power**: implement tickless idle or peripheral clock gating; measure and report current savings
- **Security**: implement a TrustZone-M partition OR configure the MPU for task isolation
- **Multi-core**: use both CM33 cores with MU-based IPC (AMP configuration)
- **Zephyr**: port the application to Zephyr with Device Tree sensor and Kconfig configuration

</v-clicks>

</div>

---
layout: two-cols
layoutClass: gap-6
---

# Suggested Project Ideas

<v-clicks>

**A — Real-time sensor fusion system**
- ADC sampling at 1 kHz (DMA + task notification)
- FIR filter task on CM33_1 (dual-core AMP)
- Control output via PWM, telemetry via UART
- Schedulability proof + DWT measurements

**B — Secure IoT gateway**
- TrustZone-M: crypto key storage in Secure world
- NS FreeRTOS: MQTT client + sensor tasks
- MPU task isolation
- Stack sizing + static allocation for Secure tasks

**C — Power-optimised data logger**
- Sensor polling with tickless idle (< 500 µA average)
- SPI flash write with event-group synchronisation
- SD card write coalescing
- Energy measurement over 24-hour simulation

</v-clicks>

::right::

<div class="mt-6 px-5 py-4 rounded-lg bg-amber-50 dark:bg-amber-900/30 text-sm leading-relaxed">

**D — Motor controller (advanced)**

- Inner loop: PWM update at 20 kHz (interrupt, no RTOS overhead)
- Outer loop: speed PID task at 1 kHz
- Safety monitor: watchdog + fault task
- ASIL-B design rationale document

**E — Zephyr IoT node**

- BME280 + LIS2DH via Device Tree
- BLE notifications (Zephyr BT stack)
- Power management: K_SLEEP + System Power Management
- west build + flash, size comparison vs FreeRTOS

<div class="mt-3 text-xs opacity-70">
Your own idea is welcome — discuss with the instructor before Week 14 if proposing a custom topic.
</div>

</div>

---
layout: section
---

# Evaluation

## Grading Rubric

---

# Presentation Rubric

<div class="mt-4 text-sm">

| Criterion | Weight | Excellent (4) | Satisfactory (2–3) | Needs work (0–1) |
|-----------|--------|--------------|-------------------|-----------------|
| **Technical depth** | 30% | All design decisions justified with theory; analysis correct | Most decisions justified; minor gaps | Shallow; no analysis |
| **Implementation** | 25% | Working demo on hardware; all features functional | Partial demo; most features work | Not running or major bugs |
| **Measurement & verification** | 20% | WCET/utilisation/power measured and compared to theoretical bounds | Some measurements; limited analysis | No measurements |
| **Clarity** | 15% | Clear structure; figures; audience can follow | Mostly clear; some confusion | Hard to follow |
| **Q&A** | 10% | Confident, correct answers; acknowledges limits honestly | Partially correct; some hesitation | Incorrect or unprepared |

</div>

---

# Report Requirements

The written report (submitted as PDF before class) must contain:

<v-clicks>

1. **System description**: block diagram, task list with periods/priorities/stack sizes
2. **Schedulability analysis**: utilisation calculation + RTA or EDF feasibility test
3. **IPC design**: which primitives, why, and how you verified correctness
4. **Memory analysis**: heap usage, stack high-water-marks, linker map `.bss`/`.data`/`.text` sizes
5. **WCET measurements**: DWT results table — measured vs claimed budget
6. **Power analysis** (if applicable): current before/after optimisation, methodology
7. **Reflection**: what worked, what was surprising, what you would change

</v-clicks>

<div v-click class="mt-4 text-sm px-4 py-2 border-l-4 border-green-500 bg-green-50 dark:bg-green-900/20">
Maximum 20 pages (excluding appendices with source code). Source code should be submitted separately via the course repository.
</div>

---
layout: section
---

# Course Summary

## What We Covered — 16 Weeks

---

# Course Learning Outcomes — Verified

<div class="mt-4 text-sm">

| CLO | Description | Weeks covered |
|-----|------------|--------------|
| **CLO 1** | Apply scheduling theory to analyse real-time task sets | 1–5 |
| **CLO 2** | Design safe task interactions using RTOS synchronisation primitives | 6–8 |
| **CLO 3** | Analyse synchronisation correctness and bound blocking | 7–8 |
| **CLO 4** | Verify memory and timing in resource-constrained RTOS applications | 9–10 |
| **CLO 5** | Evaluate security, power, and multi-core constraints on modern RTOS platforms | 11–14 |

</div>

<div v-click class="mt-4 text-sm px-4 py-2 border-l-4 border-blue-700 bg-blue-50 dark:bg-blue-900/20">
All five CLOs are demonstrated in the final project. Your project report is the evidence artefact for CLO achievement.
</div>

---

# The Journey — 16 Weeks

<div class="grid grid-cols-2 gap-4 mt-4 text-sm">

<div>

**Module 1 — Foundations (Weeks 1–2)**
Introduction to RTOS, FreeRTOS basics, task lifecycle

**Module 2 — Scheduling Theory (Weeks 3–5)**
RMS, EDF, RTA, aperiodic servers

**Module 3 — Synchronisation (Weeks 6–8)**
IPC primitives, priority inversion, deadlock, watchdogs

**Module 4 — System Internals (Weeks 9–10)**
WCET measurement, interrupt latency, memory management

</div>

<div>

**Module 5 — Advanced Hardware (Weeks 11–12)**
TrustZone-M security, multi-core AMP/SMP

**Module 6 — Power & Portability (Weeks 13–14)**
Tickless idle, low-power design, Zephyr RTOS

**Module 7 — Frontiers (Weeks 15–16)**
Mixed-criticality, safety standards, RT Linux, project presentations

</div>

</div>

<div v-click class="mt-4 text-sm px-4 py-2 border-l-4 border-amber-500 bg-amber-50 dark:bg-amber-900/20">
From "what is an RTOS?" in Week 1 to implementing TrustZone-M security partitions and analysing mixed-criticality scheduling in Week 15 — you have covered the full engineering depth of modern embedded real-time systems.
</div>

---
layout: statement
---

# Build systems that meet their deadlines.

<div class="mt-8 text-base opacity-80 max-w-2xl mx-auto">
Real-time systems engineering is the discipline of making promises — and keeping them. A deadline is a contract with physics, safety, and the people who depend on your system. The tools and theory from this course give you the means to analyse, verify, and defend those promises.
</div>

---
layout: end
class: text-center
---

# Real-Time Operating Systems

## Course Complete

<div class="mt-6 text-sm opacity-70">
KMUTNB · Faculty of Engineering · M.Eng. in Electrical & Computer Engineering<br/>
Thank you for 16 weeks of rigorous engineering.
</div>

<div class="mt-8 text-xs opacity-50">
Real-Time Operating Systems · KMUTNB · M.Eng. ECE
</div>

<style>
:root { --slidev-theme-primary: #003874; }
.slidev-layout h1 { color: #003874; }
.dark .slidev-layout h1 { color: #7ba7d9; }
table { font-size: 0.85em; }
</style>
