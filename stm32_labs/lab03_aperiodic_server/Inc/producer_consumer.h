#ifndef PRODUCER_CONSUMER_H
#define PRODUCER_CONSUMER_H

#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"

typedef struct {
    uint32_t sensor_id;
    uint32_t raw;
    uint32_t timestamp_ms;
} SensorReading_t;

/* Event bits for Part C */
#define EV_SENSOR1_READY  (1u << 0)
#define EV_SENSOR2_READY  (1u << 1)
#define EV_LOG_DONE       (1u << 2)

extern QueueHandle_t      xPipelineQueue;
extern EventGroupHandle_t xPipelineEvents;

void vSensor1Task (void *pvParameters);
void vSensor2Task (void *pvParameters);
void vLoggerTask  (void *pvParameters);
void vMonitorTask (void *pvParameters);

#endif
