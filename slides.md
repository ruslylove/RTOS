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
routeAlias: toc
---

# Course Contents

<div class="text-xs opacity-60 -mt-1 mb-3">
<span class="inline-block w-4 border-b-2 border-green-500 align-middle"></span> included in this build &nbsp;·&nbsp;
<span class="inline-block w-4 border-b-2 border-gray-300 align-middle"></span> on the syllabus roadmap
</div>

<div class="grid grid-cols-2 gap-x-10 text-sm">

<div>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-1 mb-1">Module 1 · Foundations</div>
<Link to="week-1" class="toc-link pl-3 py-0.5"><b>Week 1</b> — Foundations of Real-Time Systems</Link>
<Link to="week-2" class="toc-link pl-3 py-0.5"><b>Week 2</b> — The Task Model &amp; the Utilization Bound</Link>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-2 mb-1">Module 2 · Real-Time Scheduling Theory</div>
<Link to="week-3" class="toc-link pl-3 py-0.5"><b>Week 3</b> — Rate Monotonic Scheduling</Link>
<Link to="week-4" class="toc-link pl-3 py-0.5"><b>Week 4</b> — Earliest Deadline First; RMS vs. EDF</Link>
<Link to="week-5" class="toc-link pl-3 py-0.5"><b>Week 5</b> — Aperiodic &amp; Sporadic Servers</Link>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-2 mb-1">Module 3 · Synchronisation &amp; IPC</div>
<Link to="week-6" class="toc-link pl-3 py-0.5"><b>Week 6</b> — Queues, Semaphores, Mutexes</Link>
<Link to="week-7" class="toc-link pl-3 py-0.5"><b>Week 7</b> — Priority Inversion; PIP / PCP / SRP</Link>
<Link to="week-8" class="toc-link pl-3 py-0.5"><b>Week 8</b> — Deadlock, Livelock, Starvation</Link>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-2 mb-1">Module 4 · Timing Analysis &amp; Memory</div>
<Link to="week-9" class="toc-link pl-3 py-0.5"><b>Week 9</b> — WCET; Interrupt Latency</Link>
<Link to="week-10" class="toc-link pl-3 py-0.5"><b>Week 10</b> — FreeRTOS Memory Management</Link>

</div>

<div>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-1 mb-1">Module 5 · Advanced Hardware</div>
<Link to="week-11" class="toc-link pl-3 py-0.5"><b>Week 11</b> — ARM TrustZone-M Security</Link>
<Link to="week-12" class="toc-link pl-3 py-0.5"><b>Week 12</b> — Multi-core AMP vs. SMP</Link>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-2 mb-1">Module 6 · Power &amp; Zephyr RTOS</div>
<Link to="week-13" class="toc-link pl-3 py-0.5"><b>Week 13</b> — Low-Power RTOS; Tickless Idle</Link>
<Link to="week-14" class="toc-link pl-3 py-0.5"><b>Week 14</b> — Zephyr RTOS; Kconfig &amp; west</Link>

<div class="font-semibold text-[#003874] dark:text-[#7ba7d9] mt-2 mb-1">Module 7 · Research &amp; Final Project</div>
<Link to="week-15" class="toc-link pl-3 py-0.5"><b>Week 15</b> — Research Frontiers</Link>
<Link to="week-16" class="toc-link pl-3 py-0.5"><b>Week 16</b> — Final Project Presentations</Link>

<div class="mt-5 px-4 py-3 rounded-lg bg-blue-50 dark:bg-blue-900/30 text-xs leading-relaxed">
<b>7 modules · 16 weeks · 11 labs.</b> Each week is a self-contained deck
(<code>weekNN.md</code>) imported into this master file — this build currently
carries <b>Weeks 1–16</b>.
</div>

</div>

</div>

<style>
:root { --slidev-theme-primary: #740004ff; }
.slidev-layout h1 { color: #003874; }
.dark .slidev-layout h1 { color: #7ba7d9; }

/* Clickable week rows in the Course Contents.
   `border` is set explicitly to override the theme's dashed `a` underline. */
.toc-link {
  display: block;
  color: inherit !important;
  text-decoration: none !important;
  border: 0 solid #22c55e !important;
  border-left-width: 2px !important;
  border-radius: 0 0.25rem 0.25rem 0;
  cursor: pointer;
  transition: background-color 0.15s ease;
}
.toc-link:hover {
  background-color: rgba(34, 197, 94, 0.16);
}
.dark .toc-link:hover {
  background-color: rgba(34, 197, 94, 0.22);
}
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

---
src: ./week05.md
---

---
src: ./week06.md
---

---
src: ./week07.md
---

---
src: ./week08.md
---

---
src: ./week09.md
---

---
src: ./week10.md
---

---
src: ./week11.md
---

---
src: ./week12.md
---

---
src: ./week13.md
---

---
src: ./week14.md
---

---
src: ./week15.md
---

---
src: ./week16.md
---
