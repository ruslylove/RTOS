# Lab 07 — Stack Overflow Detection

**Platform:** WeAct STM32H503CBT6 · **Estimated time:** 2 hours

## Objectives
1. Enable FreeRTOS **stack overflow checking** (Method 2).
2. Observe the overflow hook fire as `vDangerTask` recurses deeper.
3. Use `uxTaskGetStackHighWaterMark` to right-size task stacks.
4. Allocate a task with a **static stack buffer**.

## FreeRTOS Stack Protection Methods

| Method | `configCHECK_FOR_STACK_OVERFLOW` | How it works |
|--------|----------------------------------|-------------|
| None | 0 | No check — overflow corrupts silently |
| Method 1 | 1 | Checks SP at context switch (misses slow overflows) |
| Method 2 | 2 | Paints last 16 bytes 0xA5 at creation; checks at switch |

This lab uses **Method 2** (default in `FreeRTOSConfig.h`).

## Expected Output

```
[Safe] watermark=480 words
[Danger] recursion depth=1   watermark=80 words
[Danger] recursion depth=2   watermark=62 words
...
── Task Stack Watermarks ──
Task          State  Prio  Stack   Num
Safe          B      2     480     3
Danger        R      1     12      4
Monitor       R      3     460     5
Idle          R      0     115     1
Tmr Svc       B      7     230     2

[Danger] recursion depth=7   watermark=4 words
[FATAL] STACK OVERFLOW! Task: Danger
```

## Experiments

### Exp 1 — Watermark monitoring
Run the lab as-is and record the watermark for `vSafeTask` after 30 s.
> **Q1:** What is the minimum safe stack depth for `vSafeTask`?  
> Using `uxTaskGetStackHighWaterMark`, how much margin remains?  
> Apply a 1.5× safety factor — what depth do you recommend?

### Exp 2 — Method 1 vs Method 2
Change `configCHECK_FOR_STACK_OVERFLOW` to 1. Does the hook still fire?  
Under what conditions can Method 1 miss an overflow that Method 2 catches?

> **Q2:** Describe a scenario where the stack overflows but Method 1 does NOT detect it.

### Exp 3 — Static stack allocation
In `main.c`, call `create_static_task()` from `stack_sizing.c` to launch  
`vStaticAllocTask` with a 256-word static buffer.
> **Q3:** What are the advantages of static vs dynamic stack allocation  
> in a safety-critical real-time system?

## Deliverables
| # | Item |
|---|------|
| 1 | Serial log showing watermarks shrinking and overflow hook firing |
| 2 | Recommended stack sizes with safety margins (Exp 1) |
| 3 | Written answers to Q1–Q3 |
