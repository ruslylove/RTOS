---
# Master deck — Slidev reads deck-level config (theme, fonts, …) ONLY from this
# entry file. The cover + contents slides live here; each week is imported below
# with `src: ./weekNN.md`. To add a week: author weekNN.md, add a row to the
# Course Contents slide, then add a matching `src:` block at the end of this file.
theme: seriph
background: https://cover.sli.dev
title: "Real-Time Operating Systems — Lecture Series"
info: |
  ## Real-Time Operating Systems
  Complete graduate lecture series — KMUTNB, M.Eng. ECE

  Master deck: cover + contents, then every weekNN.md imported in order.
class: text-center
transition: slide-left
mdc: true

---

# Real-Time Operating Systems

## Graduate Lecture Series

Theory &amp; hands-on RTOS engineering on dual-core ARM Cortex-M33

<div class="pt-10 opacity-70 text-sm">
  KMUTNB · Faculty of Engineering · M.Eng. in Electrical &amp; Computer Engineering
</div>

<div class="abs-br m-6 text-xs opacity-50">
  FreeRTOS · Zephyr · NXP FRDM-MCXN236
</div>

<!--
Master deck. To present or build a single week on its own, run it directly —
each weekNN.md keeps its own headmatter:  npm run slide -- week02.md
-->

<style>
:root { --slidev-theme-primary: #003874; }
.slidev-layout h1 { color: #003874; }
.dark .slidev-layout h1 { color: #7ba7d9; }
</style>

---
layout: default
---

# Course Contents

<div class="text-xs opacity-60 -mt-1 mb-3">
<span class="inline-block w-4 border-b-2 border-green-500 align-middle"></span> included in this build &nbsp;·&nbsp;
<span class="inline-block w-4 border-b-2 border-gray-300 align-middle"></span> on the syllabus roadmap
</div>

<div class="grid grid-cols-2 gap-x-10 text-sm">

<div>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-1 mb-1">Module 1 · Foundations</div>
<div class="pl-3 py-0.5 border-l-2 border-green-500"><b>Week 1</b> — Foundations of Real-Time Systems</div>
<div class="pl-3 py-0.5 border-l-2 border-green-500"><b>Week 2</b> — The Task Model &amp; the Utilization Bound</div>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-2 mb-1">Module 2 · Real-Time Scheduling Theory</div>
<div class="pl-3 py-0.5 border-l-2 border-green-500"><b>Week 3</b> — Rate Monotonic Scheduling</div>
<div class="pl-3 py-0.5 border-l-2 border-green-500"><b>Week 4</b> — Earliest Deadline First; RMS vs. EDF</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 5 — Aperiodic &amp; Sporadic Servers</div>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-2 mb-1">Module 3 · Synchronisation &amp; IPC</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 6 — Queues, Semaphores, Mutexes</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 7 — Priority Inversion; PIP / PCP / SRP</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 8 — Deadlock, Livelock, Starvation</div>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-2 mb-1">Module 4 · Timing Analysis &amp; Memory</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 9 — WCET; Interrupt Latency</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 10 — FreeRTOS Memory Management</div>

</div>

<div>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-1 mb-1">Module 5 · Advanced Hardware</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 11 — ARM TrustZone-M Security</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 12 — Multi-core AMP vs. SMP</div>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-2 mb-1">Module 6 · Power &amp; Zephyr RTOS</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 13 — Low-Power RTOS; Tickless Idle</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 14 — Zephyr RTOS; Kconfig &amp; west</div>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-2 mb-1">Module 7 · Research &amp; Final Project</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 15 — Research Frontiers</div>
<div class="pl-3 py-0.5 border-l-2 border-gray-300 dark:border-gray-700 opacity-55">Week 16 — Final Project Presentations</div>

<div class="mt-5 px-4 py-3 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-xs leading-relaxed">
<b>7 modules · 16 weeks · 11 labs.</b> Each week is a self-contained deck
(<code>weekNN.md</code>) imported into this master file — this build currently
carries <b>Weeks 1–4</b>.
</div>

</div>

</div>

<style>
:root { --slidev-theme-primary: #740004ff; }
.slidev-layout h1 { color: #003874; }
.dark .slidev-layout h1 { color: #7ba7d9; }
</style>

---
src: ./week01.md
---

---
src: ./week02.md
---

---
src: ./week03.md
---

---
src: ./week04.md
---
