#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "stream_buffer.h"
#include "ipc_demo.h"

extern void uart_lock(void);
extern void uart_unlock(void);

/* ── Part A handles (created in main.c) ─────────────────────────────────── */
EventGroupHandle_t xSensorEvents;

/* ── Part B handle (consumer task handle for direct notification) ─────────── */
TaskHandle_t xConsumerHandle;

/* ── Part C handle (created in main.c) ──────────────────────────────────── */
StreamBufferHandle_t xLogStream;

/* ════════════════════════════════════════════════════════════════════════════
 * PART A — Event Groups
 *
 * vSensorATask and vSensorBTask independently acquire data and set their
 * respective event bits.  vFusionTask waits until BOTH bits are set
 * (xWaitForAllBits = pdTRUE) — this is a synchronisation barrier / join.
 *
 * Key API:
 *   xEventGroupSetBits()   — set one or more bits (from task or ISR)
 *   xEventGroupWaitBits()  — block until a bit pattern is satisfied
 * ════════════════════════════════════════════════════════════════════════════ */

void vSensorATask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t count = 0;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(300));   /* simulate 300 ms acquisition */
        count++;
        xEventGroupSetBits(xSensorEvents, EV_SENSOR_A);
        uart_lock();
        printf("[EvtGrp] SensorA #%lu data ready — set EV_SENSOR_A\r\n",
               (unsigned long)count);
        uart_unlock();
    }
}

void vSensorBTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t count = 0;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));   /* simulate 500 ms acquisition */
        count++;
        xEventGroupSetBits(xSensorEvents, EV_SENSOR_B);
        uart_lock();
        printf("[EvtGrp] SensorB #%lu data ready — set EV_SENSOR_B\r\n",
               (unsigned long)count);
        uart_unlock();
    }
}

void vFusionTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t fusions = 0;

    for (;;) {
        /*
         * Block until BOTH EV_SENSOR_A and EV_SENSOR_B are set.
         * xClearOnExit = pdTRUE: the group atomically clears both bits
         * after returning, resetting for the next cycle.
         * xWaitForAllBits = pdTRUE: AND-semantics (need all specified bits).
         */
        EventBits_t bits = xEventGroupWaitBits(
            xSensorEvents,
            EV_ALL_READY,   /* bits to wait for */
            pdTRUE,         /* clear on exit    */
            pdTRUE,         /* wait for ALL     */
            portMAX_DELAY);

        if ((bits & EV_ALL_READY) == EV_ALL_READY) {
            uart_lock();
            printf("[EvtGrp] FUSION #%lu — both sensors ready, fusing data\r\n\r\n",
                   (unsigned long)++fusions);
            uart_unlock();
        }
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * PART B — Task Notifications
 *
 * xTaskNotify() writes directly into the target task's TCB notification value
 * — no queue, no semaphore heap object.  Ideal for ISR→task signaling.
 *
 * Key API:
 *   xTaskNotify(handle, value, eAction)  — send a value/signal
 *   xTaskNotifyWait(...)                 — block until notified
 *
 * eSetValueWithOverwrite: store value into TCB; overwrites any pending value.
 * For a pure signal (no value), use xTaskNotifyGive / ulTaskNotifyTake.
 * ════════════════════════════════════════════════════════════════════════════ */

void vProducerTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t value = 0;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(400));
        value += 10;
        xTaskNotify(xConsumerHandle, value, eSetValueWithOverwrite);
        uart_lock();
        printf("[Notif ] Producer sent value=%lu\r\n", (unsigned long)value);
        uart_unlock();
    }
}

void vConsumerTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t received;

    for (;;) {
        /*
         * Block indefinitely until a notification arrives.
         * ulBitsToClearOnEntry = 0 (don't clear notification value on entry)
         * ulBitsToClearOnExit  = 0xFFFFFFFF (clear all bits after reading)
         */
        xTaskNotifyWait(0, 0xFFFFFFFFU, &received, portMAX_DELAY);
        uart_lock();
        printf("[Notif ] Consumer received value=%lu\r\n\r\n",
               (unsigned long)received);
        uart_unlock();
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * PART C — Stream Buffer
 *
 * A stream buffer transfers a raw byte stream (not discrete messages) from
 * writer to reader.  The reader wakes only when ≥ STREAM_TRIGGER bytes are
 * available, reducing context switches for high-bandwidth streaming.
 *
 * Key API:
 *   xStreamBufferSend()    — write bytes (blocks when full)
 *   xStreamBufferReceive() — read bytes (blocks until trigger level met)
 * ════════════════════════════════════════════════════════════════════════════ */

static const char * const log_msgs[] = {
    "BOOT: system initialised\r\n",
    "SENSOR: temperature=24.3C\r\n",
    "TASK: watchdog petted\r\n",
    "NET: packet received len=64\r\n",
    "ADC: voltage=3.27V\r\n",
};
#define NUM_MSGS (sizeof(log_msgs) / sizeof(log_msgs[0]))

void vLogWriterTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t idx = 0;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(200));
        const char *msg = log_msgs[idx % NUM_MSGS];
        size_t len = strlen(msg);

        /*
         * Write bytes into the stream buffer.  If the buffer is full,
         * block for up to 100 ms then try again next cycle.
         */
        size_t sent = xStreamBufferSend(xLogStream,
                                        (const void *)msg, len,
                                        pdMS_TO_TICKS(100));
        if (sent != len) {
            uart_lock();
            printf("[Stream] Writer: buffer full, dropped %u bytes\r\n",
                   (unsigned)(len - sent));
            uart_unlock();
        }
        idx++;
    }
}

void vLogReaderTask(void *pvParameters)
{
    (void)pvParameters;
    static uint8_t rx_buf[STREAM_BUF_SIZE];

    for (;;) {
        /*
         * Block until at least STREAM_TRIGGER bytes are available
         * (set when the stream buffer was created).  Read up to buf size.
         */
        size_t received = xStreamBufferReceive(xLogStream,
                                               rx_buf,
                                               sizeof(rx_buf) - 1U,
                                               portMAX_DELAY);
        if (received > 0U) {
            rx_buf[received] = '\0';
            uart_lock();
            printf("[Stream] Reader got %u bytes:\r\n%s", (unsigned)received, rx_buf);
            uart_unlock();
        }
    }
}
