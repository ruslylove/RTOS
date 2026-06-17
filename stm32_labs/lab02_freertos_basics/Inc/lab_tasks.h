#ifndef LAB_TASKS_H
#define LAB_TASKS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

typedef struct {
    uint32_t channel;
    uint32_t raw_value;    /* simulated ADC reading 0-4095 */
    uint32_t timestamp_ms;
} SensorReading_t;

/* Kernel objects (created in main, used by tasks) */
extern QueueHandle_t     xSensorQueue;
extern SemaphoreHandle_t xUartMutex;
extern SemaphoreHandle_t xStatusSem;

void vSensorTask (void *pvParameters);
void vDisplayTask(void *pvParameters);
void vStatusTask (void *pvParameters);
void vStatusTimerCallback(TimerHandle_t xTimer);

#endif /* LAB_TASKS_H */
