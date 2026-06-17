#ifndef DEADLOCK_TASKS_H
#define DEADLOCK_TASKS_H

/* Tasks that eventually stop kicking to simulate a deadlock / hang */
void vHealthyTask   (void *pvParameters);  /* always kicks sw watchdog */
void vDeadlockTask  (void *pvParameters);  /* stops kicking after 10 s */
void vWatchdogMonitor(void *pvParameters); /* checks all sw watchdogs */

#endif
