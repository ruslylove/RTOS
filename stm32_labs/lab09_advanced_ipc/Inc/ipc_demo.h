#ifndef IPC_DEMO_H
#define IPC_DEMO_H

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "stream_buffer.h"

/*
 * All three IPC mechanisms run simultaneously.
 * Each task family is tagged in its output: [EvtGrp] [Notif] [Stream]
 */

/* ── Part A: Event Groups — multi-event AND barrier (sensor fusion) ──────── */
#define EV_SENSOR_A  (1U << 0)
#define EV_SENSOR_B  (1U << 1)
#define EV_ALL_READY (EV_SENSOR_A | EV_SENSOR_B)

extern EventGroupHandle_t xSensorEvents;

void vSensorATask(void *pvParameters);   /* sets EV_SENSOR_A every 300 ms  */
void vSensorBTask(void *pvParameters);   /* sets EV_SENSOR_B every 500 ms  */
void vFusionTask (void *pvParameters);   /* waits for BOTH bits (AND-wait)  */

/* ── Part B: Task Notifications — direct-to-task signaling ──────────────── */
extern TaskHandle_t xConsumerHandle;

void vProducerTask(void *pvParameters);  /* xTaskNotify every 400 ms        */
void vConsumerTask(void *pvParameters);  /* xTaskNotifyWait (blocks on TCB) */

/* ── Part C: Stream Buffer — byte-stream producer/consumer ───────────────── */
#define STREAM_BUF_SIZE   256U  /* total stream buffer capacity             */
#define STREAM_TRIGGER     16U  /* min bytes before reader is unblocked     */

extern StreamBufferHandle_t xLogStream;

void vLogWriterTask(void *pvParameters); /* writes variable-length frames    */
void vLogReaderTask(void *pvParameters); /* drains and prints the stream     */

#endif /* IPC_DEMO_H */
