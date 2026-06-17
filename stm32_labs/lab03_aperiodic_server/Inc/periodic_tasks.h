#ifndef PERIODIC_TASKS_H
#define PERIODIC_TASKS_H

void vPeriodic1Task(void *pvParameters); /* T=500 ms, high prio */
void vPeriodic2Task(void *pvParameters); /* T=1000 ms, mid prio */

#endif
