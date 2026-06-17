# Lab 09 — Advanced IPC: Event Groups, Task Notifications & Stream Buffers

**Platform:** WeAct STM32H503CBT6 · **Estimated time:** 2.5 hours

## Objectives
1. Use **Event Groups** to synchronise multiple tasks at a barrier (AND-wait).
2. Replace a binary semaphore with a **Task Notification** for lower overhead.
3. Transfer a byte stream between tasks using a **Stream Buffer** with a trigger level.
4. Compare the trade-offs between these three IPC mechanisms.

## IPC Mechanism Comparison

| Mechanism | Heap object? | Value passed? | Multiple waiters? | ISR safe? |
|-----------|:---:|:---:|:---:|:---:|
| Queue           | Yes | Yes (typed) | Yes (FIFO)  | Yes |
| Binary semaphore | Yes | No          | Yes         | Yes |
| Event Group     | Yes | Bits only   | Yes         | Yes |
| Task Notification | **No** (in TCB) | Yes (uint32) | **No** (1:1) | Yes |
| Stream Buffer   | Yes | Byte stream | No (1:1)    | Yes |

## Part A — Event Groups (Sensor Fusion)

```
vSensorATask ──300ms──► xEventGroupSetBits(EV_SENSOR_A)
                                │
vSensorBTask ──500ms──► xEventGroupSetBits(EV_SENSOR_B)
                                │
                          xSensorEvents
                                │
vFusionTask ◄── wakes only when EV_SENSOR_A AND EV_SENSOR_B both set ──
                         (xWaitForAllBits = pdTRUE)
```

`xEventGroupWaitBits(group, EV_ALL_READY, pdTRUE, pdTRUE, portMAX_DELAY)`:
- `pdTRUE` (3rd arg) — clear bits on exit → resets for next cycle
- `pdTRUE` (4th arg) — AND: all specified bits must be set

The fusion task wakes at the **LCM** of 300 ms and 500 ms = 1500 ms intervals
(the first time both sensors have fired since the last clearance).

## Part B — Task Notifications

Each FreeRTOS task has a built-in 32-bit notification value in its TCB.  
No heap allocation required; lower latency than a semaphore.

```c
/* Producer sends a value directly into the consumer's TCB */
xTaskNotify(xConsumerHandle, value, eSetValueWithOverwrite);

/* Consumer blocks until notified, then reads the value */
xTaskNotifyWait(0, 0xFFFFFFFF, &received, portMAX_DELAY);
```

Limitation: only **one notifier at a time** — a second notification before the
consumer reads will overwrite the first (with `eSetValueWithOverwrite`).  
Use a queue if multiple producers need to signal the same consumer.

## Part C — Stream Buffer

A stream buffer is a contiguous ring of bytes — no message framing.  
The **trigger level** (`STREAM_TRIGGER = 16`) prevents the reader from waking
for every byte; it only unblocks when at least 16 bytes are available.

```
vLogWriterTask → xStreamBufferSend() ─────────────────────►┐
                                                            │ xLogStream (256 B)
vLogReaderTask ◄─ xStreamBufferReceive() (trigger≥16 B) ◄──┘
```

Reads return **however many bytes are available** (up to buffer size), not
necessarily a complete message — the reader must handle partial frames.

## Expected Output

```
=== RTOS Lab 09: Advanced IPC ===
Running: Event Groups | Task Notifications | Stream Buffer

[EvtGrp] SensorA #1 data ready — set EV_SENSOR_A
[Notif ] Producer sent value=10
[Notif ] Consumer received value=10

[EvtGrp] SensorB #1 data ready — set EV_SENSOR_B
[EvtGrp] FUSION #1 — both sensors ready, fusing data

[EvtGrp] SensorA #2 data ready — set EV_SENSOR_A
[Notif ] Producer sent value=20
...
[Stream] Reader got 28 bytes:
BOOT: system initialised
SENSOR: temperature=24.3C
```

## Experiments

### Exp 1 — OR-wait vs AND-wait
Change the 4th argument of `xEventGroupWaitBits` from `pdTRUE` (AND) to `pdFALSE` (OR).

> **Q1:** How does the fusion task's wakeup pattern change?  
> What is the new effective period and why?

### Exp 2 — Task notification as counting semaphore
Modify `vProducerTask` to send 3 notifications without delay between them.  
Observe how many the consumer receives.

> **Q2:** How many values does the consumer print?  
> Why does `eSetValueWithOverwrite` behave this way, and when is that acceptable?  
> What action type would you use to avoid losing notifications?

### Exp 3 — Stream buffer trigger level
Change `STREAM_TRIGGER` to `1` and rebuild.

> **Q3:** How does the reader's output change in granularity?  
> What is the trade-off between a small and a large trigger level  
> in a real serial-receive pipeline?

### Exp 4 — Choosing the right mechanism
For each scenario below, state which mechanism (queue, event group, task notification,
stream buffer) is most appropriate and why:

| Scenario | Best mechanism |
|----------|---------------|
| A UART RX ISR needs to wake a parser task with each byte | ? |
| A watchdog task needs to know that ALL 4 sensor tasks have completed their cycle | ? |
| A temperature sensor sends a single float to a display task every second | ? |
| An audio codec feeds raw PCM data to a software equaliser | ? |

## Deliverables
| # | Item |
|---|------|
| 1 | Serial log showing all three IPC mechanisms running (annotate with timestamps) |
| 2 | Predicted vs measured fusion period for Exp 1 (AND vs OR) |
| 3 | Written answers to Q1–Q4 |

## Implementation Notes

- `xConsumerHandle` must be set **before** `vProducerTask` starts sending notifications.  
  Ensure the consumer task is created first and its handle is valid.
- `xStreamBufferCreate(size, triggerLevel)` — the buffer cannot hold more than  
  `size - 1` bytes at any instant (one byte is reserved as a sentinel).
- `xStreamBufferSend` blocks if the buffer is full; `xStreamBufferSendFromISR`  
  is the interrupt-safe variant and never blocks.
- `xEventGroupSetBits` can be called from an ISR using `xEventGroupSetBitsFromISR`  
  + `portYIELD_FROM_ISR`.
