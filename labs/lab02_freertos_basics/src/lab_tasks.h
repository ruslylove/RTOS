#ifndef LAB_TASKS_H
#define LAB_TASKS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* Shared RTOS objects — created in main, used across tasks */
extern QueueHandle_t    xSensorQueue;
extern SemaphoreHandle_t xUartMutex;
extern SemaphoreHandle_t xStatusSem;

/* Sensor reading passed through the queue */
typedef struct {
    uint32_t timestamp_ms;
    int32_t  value;
} SensorReading_t;

/* Task functions */
void vSensorTask(void *pvParams);
void vDisplayTask(void *pvParams);
void vStatusTask(void *pvParams);

/* Timer callback */
void vStatusTimerCallback(TimerHandle_t xTimer);

#endif
