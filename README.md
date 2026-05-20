# RTOS Course — Lecture Slides

[Slidev](https://sli.dev) lecture decks for **Real-Time Operating Systems**
(KMUTNB, Faculty of Engineering, M.Eng. in Electrical & Computer Engineering).

One self-contained deck per course week, sharing the `seriph` theme.

## Getting started

```bash
npm install        # once
npm run dev        # opens the full course deck in the browser with live reload
```

## Project structure

The Slidev project lives in the **repository root**.

```
RTOS/
├─ slides.md          Master deck — cover + contents, imports every weekNN.md
├─ week01.md          Week 1 — Foundations of Real-Time Systems   (complete)
├─ week02.md          Week 2 — Task Model & the Utilization Bound (complete)
├─ week03.md          Week 3 — Rate Monotonic Scheduling          (complete)
├─ week04.md          Week 4 — Earliest Deadline First & RMS vs. EDF (complete)
├─ week05.md … week16.md   (to be added — see course map below)
├─ figures/           TikZ figure sources (.tex) + compiled .svg + build.sh
├─ global-bottom.vue  Global component — page numbers on every slide
├─ package.json
└─ rtos_syllabus.tex / .pdf, ref_books/   (course source material)
```

**`slides.md` is the master deck.** Slidev reads deck-level config (theme,
fonts, transition) **only** from the entry file, so `slides.md` carries that
config, then a **cover** and a **Course Contents** slide, and finally imports
every week — one `src: ./weekNN.md` block per week, in order.

Each `weekNN.md` also keeps its own headmatter, so any week can be built or
presented on its own:

```bash
npm run slide -- week02.md          # run a single week deck directly
```

## Building & exporting

```bash
npm run build                       # whole course -> dist/  (from slides.md)
npm run export                      # PDF export (needs playwright-chromium)
npm i -D playwright-chromium        # one-time, for PDF export
```

While presenting: `f` fullscreen · `o` slide overview · `d` dark mode ·
`g` go-to slide · arrow keys to navigate.

## Course map (from rtos_syllabus.tex)

| Wk | Topic | Module |
|----|-------|--------|
| 1  | RT taxonomy; hard/soft/firm; RTOS vs GPOS | 1 — Foundations |
| 2  | Task model; periodic/aperiodic/sporadic; utilization bound | 1 |
| 3  | Rate Monotonic Scheduling; Liu & Layland | 2 — Scheduling |
| 4  | Earliest Deadline First; RMS vs EDF | 2 |
| 5  | Aperiodic/sporadic servers | 2 |
| 6  | Queues, semaphores, mutexes, event groups | 3 — Sync & IPC |
| 7  | Priority inversion; PIP / PCP / SRP | 3 |
| 8  | Deadlock, livelock, starvation; watchdogs | 3 |
| 9  | WCET; DWT cycle counter; interrupt latency | 4 — Timing & Memory |
| 10 | FreeRTOS memory management; heap_1–heap_5 | 4 |
| 11 | ARM TrustZone-M; SAU; secure gateway | 5 — Advanced HW |
| 12 | Multi-core AMP vs SMP; mailbox | 5 |
| 13 | Low-power RTOS; tickless idle | 6 — Power & Zephyr |
| 14 | Zephyr RTOS; Kconfig; device tree; west | 6 |
| 15 | Research frontiers; mixed-criticality; safety standards | 7 — Research |
| 16 | Final project presentations | 7 |

## Conventions used in the decks

- **Theme** — `seriph`, with KMUTNB blue (`#003874`) as the primary accent.
- **Math** — KaTeX, `$...$` inline and `$$...$$` block.
- **Diagrams** — authored as **TikZ** in `figures/*.tex`, compiled to SVG with
  `figures/build.sh` (needs a LaTeX toolchain: `latex` + `dvisvgm`).
- **Code** — Shiki highlighting; line-by-line reveals via ```c {1|2-3|all}```.
- **Builds** — incremental reveals use `<v-clicks>` / `v-click`.
- **Image files** — keep them in `figures/` and reference them with a
  **relative** path (`./figures/name.svg`). Do **not** put them in a `public/`
  folder reached via `<img>` — Vite cannot import `public/` assets.

## Editing a diagram

```bash
# edit the TikZ source, e.g. figures/anatomy_job.tex, then:
cd figures && ./build.sh        # recompiles every *.tex to *.svg
```

To start a new week, copy `week01.md` as a template — keep the headmatter and
the `<style>` block at the end so branding stays consistent. Then register it
in `slides.md`: add a row to the **Course Contents** slide and a matching
`src: ./weekNN.md` block at the end of the file.
