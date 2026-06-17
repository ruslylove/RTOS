# Lab 02 — FreeRTOS Basics

**Platform:** WeAct STM32H503CBT6 · **Estimated time:** 2–3 hours

## Objectives
1. Use a **queue** to transfer data between tasks.
2. Use a **binary semaphore** to synchronise a task with a software timer.
3. Use a **mutex** to guard shared UART output.
4. Observe how priorities determine which task runs next.

## RTOS Primitives Demonstrated

| Primitive | Object | Purpose |
|-----------|--------|---------|
| Queue | `xSensorQueue` (depth 8) | ADC readings from Sensor → Display |
| Mutex | `xUartMutex` | Serialise `printf` from multiple tasks |
| Binary semaphore | `xStatusSem` | Timer → Status task wake-up |
| Software timer | 1000 ms auto-reload | Periodic kick for Status task |

## Tasks

| Task | Priority | Blocking call | Action |
|------|----------|--------------|--------|
| `vSensorTask` | 3 HIGH | `vTaskDelay(500 ms)` | Enqueues simulated ADC reading |
| `vDisplayTask` | 2 MED | `xQueueReceive(portMAX_DELAY)` | Prints reading from queue |
| `vStatusTask` | 1 LOW | `xSemaphoreTake(portMAX_DELAY)` | Prints heap/uptime once per second |

## Build & Flash
```bash
cd stm32_labs/lab02_freertos_basics
cmake --preset Debug && cmake --build build/Debug
```
Flash via VS Code F5 or STM32CubeProgrammer.

## Expected Output
```
[SENSOR] ch=0  raw=1024  t=503 ms
[SENSOR] ch=1  raw=1161  t=1005 ms
[STATUS] tick #1  free_heap=15200 B  uptime=1000 ms
[SENSOR] ch=2  raw=1298  t=1508 ms
...
```

## Experiments

### Exp 1 — Queue overflow
Reduce queue depth to 2 in `main.c`:
```c
xSensorQueue = xQueueCreate(2, sizeof(SensorReading_t));
```
Add `vTaskDelay(pdMS_TO_TICKS(2000))` inside `vDisplayTask` to slow the consumer.
> **Q1:** What happens when `xQueueSend` is called on a full queue?  
> What does `pdMS_TO_TICKS(10)` timeout mean for the producer?

### Exp 2 — Priority inversion preview
Set `vDisplayTask` priority to **4** (higher than `vSensorTask`).
> **Q2:** Does the Display task print immediately after each enqueue?  
> Why does it still block on `xQueueReceive`?

### Exp 3 — Semaphore vs. polling
Replace the binary semaphore + timer with a `vTaskDelay(1000)` in `vStatusTask`.
> **Q3:** What is the conceptual difference between polling with `vTaskDelay`  
> and blocking on a semaphore given by a timer?

## Deliverables
| # | Item |
|---|------|
| 1 | Serial capture showing all three tasks interleaving correctly |
| 2 | Written answers to Q1–Q3 |
| 3 | Modified code from Experiment 1 with explanation |
