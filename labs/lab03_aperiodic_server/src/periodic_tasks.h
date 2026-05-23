#ifndef PERIODIC_TASKS_H
#define PERIODIC_TASKS_H

/*
 * periodic_tasks.h
 * Background periodic tasks that simulate a real-time workload.
 *
 * tau1 (vPeriodic1Task): period 100 ms, priority 3
 * tau2 (vPeriodic2Task): period 200 ms, priority 2
 */

void vPeriodic1Task(void *pvParameters);
void vPeriodic2Task(void *pvParameters);

#endif /* PERIODIC_TASKS_H */
