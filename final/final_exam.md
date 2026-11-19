# Real-Time Operating Systems — Final Examination

**KMUTNB · Faculty of Engineering · M.Eng. in Electrical & Computer Engineering**

| | |
|---|---|
| **Course** | 010135311 — Real-Time Operating Systems |
| **Semester** | 1/2026 |
| **Coverage** | Weeks 1–5 (foundations, task model, RMS, EDF, aperiodic/sporadic servers) and Week 6 up to and including mutexes (race conditions, critical sections, queues, semaphores, mutexes) |
| **Duration** | 3 hours (180 minutes) |
| **Format** | **Open book** |
| **Total** | 100 points |

Name: ______________________________  Student ID: ____________________  Date: ____________

---

## Instructions

1. **Permitted:** lecture slides, your own notes, textbooks (Laplante, Liu, Barry), lab code and write-ups, a calculator.
2. **Not permitted:** communicating with anyone, or using the internet, AI assistants, or any online service.
3. **Show your work.** For schedulability problems write the formula, the substituted numbers, and the conclusion. A correct final answer with no working earns at most half marks. A wrong answer with correct method earns most of the marks.
4. **State any assumption** you make. Unless a question says otherwise:
   - scheduling is **preemptive, fixed-priority** (a larger priority number is *not* implied — "higher priority" is always stated in words);
   - context-switch and kernel overhead are **zero**;
   - tasks are **independent** and **synchronous** ($\Phi_i = 0$), time unit is **ms**;
   - ties in priority or deadline are broken in favour of the **lower task index**.
5. Code is FreeRTOS on a Cortex-M33 (FRDM-MCXN236). You do not need to remember exact header names; correct API names and argument logic are what matter.
6. Budget your time. A suggested allocation is printed beside each part. Do not spend more than 10 minutes stuck on one question — move on and return.

### Score summary

| Part | Topic | Points | Suggested time |
|------|-------|-------:|---------------:|
| A | Concepts | 12 | 15 min |
| B | Task model and timing metrics | 12 | 20 min |
| C | Rate Monotonic Scheduling and RTA | 14 | 25 min |
| D | EDF and RMS vs. EDF | 14 | 30 min |
| E | Aperiodic servers and sporadic tasks | 13 | 25 min |
| F | Queues, semaphores, mutexes | 20 | 35 min |
| G | Integrated design problem | 15 | 30 min |
| | **Total** | **100** | **180 min** |

### Reference values (provided)

Liu & Layland bound $U_{\text{lub}}(n) = n\left(2^{1/n}-1\right)$:

| $n$ | 1 | 2 | 3 | 4 | 5 | 6 | $\infty$ |
|---|---|---|---|---|---|---|---|
| $U_{\text{lub}}(n)$ | 1.0000 | 0.8284 | 0.7798 | 0.7568 | 0.7435 | 0.7348 | $\ln 2 = 0.6931$ |

Response-time recurrence: $R_i^{(0)} = C_i,\;\; R_i^{(k+1)} = C_i + \sum_{j \in \text{hp}(i)} \left\lceil R_i^{(k)} / T_j \right\rceil C_j$

Demand bound: $\text{dbf}(L) = \sum_i \max\!\left(0,\ \left\lfloor \dfrac{L-D_i}{T_i} \right\rfloor + 1\right) C_i \le L$

Deferrable-server bound ($n$ periodic tasks + one DS of utilization $U_s$): $U_{\text{lub}} = U_s + n\left[\left(\dfrac{U_s+2}{2U_s+1}\right)^{1/n} - 1\right]$

---

## Part A — Concepts (12 points · ~15 min)

**A1. (2 pts)** Classify each system's timing requirement as **hard**, **firm**, or **soft**, and give a one-phrase justification for each.

 a. Airbag deployment controller
 b. Video-conference frame renderer (a late frame is dropped, the next one is shown)
 c. Weather-station display refreshing every 5 s
 d. Sensor-fusion sample where a stale sample is discarded and useless, but a miss does not endanger anyone

**A2. (2 pts)** A colleague says: *"Our MCU runs at 150 MHz and averages 2 µs response to an interrupt, so it is a real-time system."*
 a. Explain why this statement is not sufficient. (1 pt)
 b. Name **two** kernel properties that distinguish an RTOS from a general-purpose OS. (1 pt)

**A3. (2 pts)** Define **periodic**, **sporadic**, and **aperiodic** tasks by their arrival pattern. Which of the three can sensibly carry a **hard** deadline *and* needs no server to be analysed, and why?

**A4. (2 pts)** Decide whether each statement is **true or false**, with one sentence of justification.

 a. "RMS is optimal, so if RMS cannot schedule a task set, no scheduling algorithm can." (1 pt)
 b. "Under the critical-instant theorem, the worst-case response time of $\tau_i$ occurs when $\tau_i$ is released at the same instant as all **lower**-priority tasks." (1 pt)

**A5. (2 pts)** Apply the utilization-bound test to each case below under RMS. Answer *schedulable*, *inconclusive*, or *infeasible*.

| | $n$ | $U$ |
|---|---|---|
| (i) | 3 | 0.75 |
| (ii) | 4 | 0.80 |
| (iii) | 2 | 1.05 |

**A6. (2 pts)** State **three** differences between a FreeRTOS **mutex** and a **binary semaphore**, and say which one you would use to protect an I²C bus shared by three tasks.

---

## Part B — Task Model and Timing Metrics (12 points · ~20 min)

**B1. (4 pts)** A job is released at $r = 6$ ms with relative deadline $D = 10$ ms and execution time $C = 4$ ms. It first gets the CPU at $t = 9$ and completes at $f = 19$.

 a. Absolute deadline and response time. (1 pt)
 b. Lateness and tardiness. (1 pt)
 c. Laxity at release, and laxity at $t = 9$ (just before it starts running). (1 pt)
 d. Another job of the same task, released at $r = 6$, completes at $f = 12$. Give its lateness and tardiness, and explain why lateness may be negative but tardiness may not. (1 pt)

**B2. (4 pts)** A task is specified by $\tau = (\Phi, T, C, D) = (3,\ 10,\ 2,\ 7)$.

 a. Give the release time and absolute deadline of its first three jobs. (1 pt)
 b. Is the deadline model *implicit*, *constrained*, or *arbitrary*? Which fixed-priority assignment rule applies to a set of such tasks? (1 pt)
 c. Three tasks have periods $T_a = 6,\ T_b = 10,\ T_c = 15$ ms. Compute the hyperperiod $H$ and the number of jobs of each task inside one hyperperiod. (2 pts)

**B3. (4 pts)** A control task must run every 10 ms. Its body takes 3 ms to execute.

```c
void vControlTask(void *pv)
{
    for (;;) {
        ReadSensorAndCompute();            /* 3 ms of work */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

 a. With a 1 kHz tick, what period does this task actually have, and what happens over many cycles if a higher-priority task occasionally preempts it? (1.5 pts)
 b. Rewrite the loop so the task is released every 10 ms without drift. (1.5 pts)
 c. After your fix, can the task still start late? Explain in terms of the timing metrics from Week 2. (1 pt)

---

## Part C — Rate Monotonic Scheduling and RTA (14 points · ~25 min)

Task set **P** (priorities assigned by RMS):

| Task | $C_i$ | $T_i = D_i$ |
|------|------:|------:|
| $\tau_1$ | 1 | 4 |
| $\tau_2$ | 2 | 6 |
| $\tau_3$ | 3 | 12 |

**C1. (3 pts)** Assign RMS priorities. Compute $U$ and $U_{\text{lub}}(3)$ and state what the utilization test concludes.

**C2. (6 pts)** Perform response-time analysis for all three tasks. **Show every iteration** of the recurrence. State whether set P is schedulable.

**C3. (3 pts)** Draw the RMS Gantt chart for $t \in [0, 12]$. Mark every release, every deadline, and every preemption. Your chart must agree with the response times from C2.

**C4. (2 pts)** Keeping $\tau_1$ and $\tau_2$ unchanged, find the **largest integer value of $C_3$** for which the set is still schedulable under RMS. Justify with RTA. Is the Liu & Layland bound a necessary condition? Explain in one sentence.

---

## Part D — EDF and RMS vs. EDF (14 points · ~30 min)

Task set **Q** (implicit deadlines):

| Task | $C_i$ | $T_i = D_i$ |
|------|------:|------:|
| $\tau_1$ | 2 | 6 |
| $\tau_2$ | 2 | 8 |
| $\tau_3$ | 5 | 12 |

**D1. (2 pts)** Compute $U$. What does the EDF theorem say about set Q? What does the Liu & Layland test say about it under RMS?

**D2. (3 pts)** Run RTA on set Q under RMS. Show the iterations and state the outcome.

**D3. (6 pts)** Construct the **EDF schedule** for set Q over one hyperperiod. Present it as a table of execution intervals (for example `[0,2] J1,1`), where $J_{i,k}$ is the $k$-th job of $\tau_i$. Then list the completion time of **every** job and confirm each meets its deadline. Use the tie-break rule from the instructions.

**D4. (3 pts)** Constrained-deadline set R:

| Task | $C_i$ | $T_i$ | $D_i$ |
|------|------:|------:|------:|
| $\tau_1$ | 3 | 8 | 4 |
| $\tau_2$ | 3 | 10 | 5 |

 a. Compute $U$. Does $U \le 1$ guarantee EDF schedulability here? (1 pt)
 b. Compute $\text{dbf}(4)$ and $\text{dbf}(5)$. (1 pt)
 c. Is set R EDF-schedulable? Explain what goes wrong in physical terms (what must happen before $t = 5$?). (1 pt)

---

## Part E — Aperiodic Servers and Sporadic Tasks (13 points · ~25 min)

**E1. (5 pts)** Periodic tasks $\tau_1 = (C{=}1, T{=}5)$ and $\tau_2 = (C{=}2, T{=}10)$ share the CPU with an aperiodic server of capacity $C_s = 1$ and period $T_s = 4$. Priorities are by RMS, so the server has the **highest** priority.

 a. If the server is a **polling server**, apply the Liu & Layland test with the server counted as a periodic task. What is the verdict? (2 pts)
 b. If the server is a **deferrable server**, compute the DS utilization bound for $n = 2$ periodic tasks and $U_s = C_s/T_s$. Is the set still guaranteed schedulable? Why can the ordinary $U_{\text{lub}}(3)$ not be used? (3 pts)

**E2. (3 pts)** In the system of E1, an aperiodic request needing 1 ms of service arrives at $t = 1$. The server's poll at $t = 0$ found the queue empty.

 a. When does the request complete, and what is its response time, under the **polling server**? (1 pt)
 b. Same question under the **deferrable server**. (1 pt)
 c. The **sporadic server** gives the same response time as (b). Why can it nevertheless be analysed as an ordinary periodic task while the deferrable server cannot? (1 pt)

**E3. (5 pts)** A system has three activities:

| Activity | Behaviour | $C$ | Period / min. inter-arrival | $D$ |
|---|---|---:|---:|---:|
| Control loop $\tau_1$ | periodic | 2 | $T = 10$ | 10 |
| Fault alarm $\tau_2$ | **sporadic** | 2 | $a = 20$ | **3** |
| Logger $\tau_3$ | periodic | 3 | $T = 20$ | 20 |

 a. How do you model the sporadic alarm for analysis, and why is this safe for any slower arrival pattern? (1 pt)
 b. Which priority-assignment rule must you use, and what is the resulting priority order? (1 pt)
 c. Run RTA for all three tasks with that assignment. Is the set schedulable? (2 pts)
 d. Show that plain **RMS** would have failed for the alarm. (1 pt)

---

## Part F — Queues, Semaphores, Mutexes (20 points · ~35 min)

**F1. (5 pts)** The code below logs bytes received by a UART ISR; the logger and a heartbeat task both print on the same UART and are meant to be serialised by a mutex. It contains **at least five distinct defects** (functional bugs or incorrect API use). Find **five**, explain the failure each causes, and give the fix. (1 pt each.)

```c
static SemaphoreHandle_t xUartMutex;
static QueueHandle_t     xRxQueue;

void UART_IRQHandler(void)
{
    uint8_t b = UART_ReadByte();
    xQueueSendToBack(xRxQueue, &b, portMAX_DELAY);
}

void vLoggerTask(void *pv)
{
    uint8_t b;
    for (;;) {
        xQueueReceive(xRxQueue, &b, portMAX_DELAY);
        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        PRINTF("rx=0x%02x\r\n", b);
        if (b == 0x1B) {                                     /* ESC */
            PRINTF("escape received\r\n");
            continue;
        }
        xSemaphoreGive(xUartMutex);
    }
}

void vHeartbeatTask(void *pv)
{
    for (;;) {
        PRINTF("alive\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    BOARD_InitHardware();
    xRxQueue   = xQueueCreate(8, sizeof(uint32_t));
    xUartMutex = xSemaphoreCreateBinary();
    xTaskCreate(vLoggerTask,    "log", 256, NULL, 1, NULL);
    xTaskCreate(vHeartbeatTask, "hb",  256, NULL, 1, NULL);
    vTaskStartScheduler();
    for (;;) {}
}
```

**F2. (5 pts)** For each scenario choose the **most appropriate primitive** from: *queue*, *binary semaphore*, *counting semaphore*, *mutex*, *critical section*. Give a one-sentence reason and name the key API call(s). (1 pt each.)

 a. A DMA-complete ISR must wake a handler task. No data is passed.
 b. Three tasks take turns using one I²C bus; each transaction lasts about 200 µs of task-level code.
 c. A pool of four identical DMA channels is shared by many tasks.
 d. A sensor task sends a 12-byte struct `{timestamp, x, y}` to a filter task. Bursts of up to three samples can occur while the filter is busy.
 e. A timer ISR and a task both execute `gTicks++` on a 32-bit variable. The task's access is two instructions long.

**F3. (5 pts)** A queue `q` of length **2** carries `int` items. Task **P** has priority **2** and task **C** has priority **1** (higher number = higher priority for this question only). Both are created before the scheduler starts. Assume all calls other than blocking and `vTaskDelay` take zero time, and `PRINTF` is instantaneous.

```c
void vP(void *pv) {                  void vC(void *pv) {
    for (int i = 1; i <= 4; i++)         for (;;) {
        xQueueSendToBack(q, &i,              int v;
                         portMAX_DELAY);     xQueueReceive(q, &v, portMAX_DELAY);
    vTaskDelete(NULL);                       PRINTF("%d @%u\n", v, xTaskGetTickCount());
}                                            vTaskDelay(10);
                                         }
                                     }
```

 a. Give the exact output (value and tick) printed by C. (2 pts)
 b. What are the contents of the queue at tick 5? (1 pt)
 c. State at which instants task P blocks and unblocks, and explain why C is preempted inside its first `xQueueReceive` call. (2 pts)

**F4. (5 pts)**

 a. Two tasks each execute `gCounter++` once on a shared `volatile uint32_t` with initial value 10. On a load-store architecture the increment is `LDR r0,[gCounter]; ADD r0,r0,#1; STR r0,[gCounter]`. Write an interleaving of the six instructions that leaves `gCounter == 11`. Say where the preemption happened. (2 pts)

 b. A function `vLog()` takes `xMutex` (created with `xSemaphoreCreateMutex()`) and, while holding it, calls `vFormat()`, which also takes `xMutex`. What happens when a task runs this code with `portMAX_DELAY`? Give **two** distinct ways to fix it. (2 pts)

 c. Why can a mutex not be given from an ISR, and why is a binary semaphore (not a mutex) the right object for ISR → task signalling? (1 pt)

---

## Part G — Integrated Design Problem (15 points · ~30 min)

You are designing the firmware for a small motor-drive controller. Ignore context-switch cost and ignore blocking caused by the shared UART mutex (those effects are analysed in the next module).

| Activity | Description | $C$ (ms) | Period / min. gap | $D$ (ms) |
|---|---|---:|---:|---:|
| **E-stop handler** | Runs when a button ISR fires; at most once per 50 ms | 1 | $a = 50$ | 5 |
| **Current loop** | Reads ADC, updates PWM | 1 | $T = 5$ | 5 |
| **CLI** | Operator commands from the serial console, arbitrary arrival, soft | — | — | — |
| **Speed loop** | Computes speed, sends a result struct to the logger | 2 | $T = 20$ | 20 |
| **Logger** | Receives results, prints them over the UART | 4 | $T = 40$ | 40 |

CLI requests are served by a **polling server** with $C_s = 2$ ms, $T_s = 10$ ms. The CLI handler and the logger both print on the same UART.

**G1. (2 pts)** For each of the five activities say whether it is periodic, sporadic, or aperiodic, and whether its deadline is hard, firm, or soft.

**G2. (3 pts)** Give the full priority order (highest to lowest) of the five schedulable entities (including the server). Name the rule you use, and justify the placement of the E-stop handler relative to the current loop (they have equal deadlines).

**G3. (2 pts)** Compute total utilization (including the server) and apply the utilization-bound test. What does it conclude?

**G4. (5 pts)** Compute the worst-case response times of the **speed loop** and of the **logger** with RTA (show the iterations). Compare each with its deadline.

**G5. (3 pts)** Identify the three inter-task communication objects this design needs. For each, state the primitive, who produces and who consumes, and which API variant (task or ISR) is called from where:

 a. button ISR → E-stop handler
 b. speed loop → logger
 c. logger and CLI handler → UART

---

*End of examination.*
