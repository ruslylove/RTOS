# RTOS Course — Lecture Slides

[Slidev](https://sli.dev) lecture decks for **Real-Time Operating Systems**
(KMUTNB, Faculty of Engineering, M.Eng. in Electrical & Computer Engineering).

One self-contained deck per course week, sharing the `seriph` theme.

## Getting started

```bash
npm install        # once
npm run dev        # opens Week 1 in the browser with live reload
```

## Presenting / building a specific week

Each week is an independent Slidev entry file. Use the `slide` script:

```bash
npm run slide -- week01.md            # live presentation (dev server)
npm run slide -- build week01.md      # static site -> dist/
npm run slide -- export week01.md     # PDF export (needs playwright-chromium)
```

PDF export requires the exporter dependency once:

```bash
npm i -D playwright-chromium
```

While presenting: `f` fullscreen · `o` slide overview · `d` dark mode ·
`g` go-to slide · arrow keys to navigate.

## Layout

```
slides/
├─ week01.md        Week 1 — Foundations of Real-Time Systems   (complete)
├─ week02.md … week16.md   (to be added — see syllabus)
├─ public/images/   shared figures referenced as /images/...
└─ package.json
```

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
- **Diagrams** — Mermaid fenced blocks; hand-drawn timing figures as inline SVG.
- **Code** — Shiki highlighting; line-by-line reveals via ```c {1|2-3|all}```.
- **Builds** — incremental reveals use `<v-clicks>` / `v-click`.

To start a new week, copy `week01.md` as a template — keep the deck headmatter
and the `<style>` block at the end so branding stays consistent.
