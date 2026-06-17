#ifndef POWER_TASKS_H
#define POWER_TASKS_H

#include <stdint.h>

/* Blinks PC13 LED and prints blink count every LED_PERIOD_MS */
#define LED_PERIOD_MS   500U
void vLedTask(void *pvParameters);

/* Simulates periodic sensor work then blocks — long block = more sleep time */
#define SENSOR_PERIOD_MS  2000U
void vSensorTask(void *pvParameters);

/* Prints idle statistics every STATS_PERIOD_MS */
#define STATS_PERIOD_MS   5000U
void vStatsTask(void *pvParameters);

/*
 * Incremented by vApplicationIdleHook each time the idle task runs.
 * Read (and reset) by vStatsTask to compute idle frequency.
 * Volatile because it is written in idle context and read in task context.
 */
extern volatile uint32_t ulIdleCount;

/* Incremented by vApplicationTickHook — measures ticks that fire while running */
extern volatile uint32_t ulTickCount;

#endif /* POWER_TASKS_H */
