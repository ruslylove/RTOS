#ifndef MPU_TASKS_H
#define MPU_TASKS_H

/*
 * Set USE_ROGUE_TASK to 1 to launch vRogueTask, which attempts a write to
 * the MPU-protected secure_buffer — this intentionally triggers a MemManage
 * fault so you can observe the fault handler output.
 *
 * Run with 0 first to confirm normal operation, then change to 1 to see the fault.
 */
#define USE_ROGUE_TASK  0

/* Reads and prints secure_buffer content every second (allowed: region is RO, not NA) */
void vNormalTask(void *pvParameters);

/* Dumps live task list every 4 s */
void vMonitorTask(void *pvParameters);

#if USE_ROGUE_TASK
/* Waits 3 s then writes to secure_buffer → MemManage fault */
void vRogueTask(void *pvParameters);
#endif

#endif /* MPU_TASKS_H */
