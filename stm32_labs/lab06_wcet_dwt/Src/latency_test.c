#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "dwt.h"
#include "workload.h"
#include "latency_test.h"

extern void uart_lock(void);
extern void uart_unlock(void);

/* ── Shared result structure ─────────────────────────────────────────────── */
static struct {
    uint32_t sqrt_cycles;
    uint32_t memcpy_cycles;
    uint32_t fir_cycles;
    uint32_t sort_cycles;
    uint32_t dot_cycles;
    uint32_t sem_latency_cycles;
} results;

static SemaphoreHandle_t xLatSem;    /* for latency measurement */
static volatile uint32_t give_time;  /* cycle count at Give */

/* ── WCET measurement task ───────────────────────────────────────────────── */
void vWcetTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        results.sqrt_cycles   = workload_sqrt(1234.5678f);
        results.memcpy_cycles = workload_memcpy_256();
        results.fir_cycles    = workload_fir_filter();
        results.sort_cycles   = workload_sort_128();
        results.dot_cycles    = workload_fpu_dot();

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ── Latency measurement: how fast does a task wake after semaphore give? ── */

/* High-priority sender — gives semaphore, records cycle count */
static void vSenderTask(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        give_time = dwt_cycles();          /* record time of give */
        xSemaphoreGive(xLatSem);
    }
}

/* High-priority receiver — measures cycles from give to wake */
void vLatencyTask(void *pvParameters)
{
    (void)pvParameters;
    xLatSem = xSemaphoreCreateBinary();

    /* Create sender at higher priority so it runs first, then yields to us */
    xTaskCreate(vSenderTask, "Sender", 256, NULL, 6, NULL);

    for (;;) {
        xSemaphoreTake(xLatSem, portMAX_DELAY);
        uint32_t wake_time = dwt_cycles();
        results.sem_latency_cycles = wake_time - give_time;
    }
}

/* ── Report task — prints results periodically ────────────────────────────── */
void vReportTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));

        uart_lock();
        printf("\r\n── WCET Results (cycles @ 250 MHz, 1 cycle = 4 ns) ──\r\n");
        printf("  sqrtf(x)         : %5lu cy = %5.1f us\r\n",
               (unsigned long)results.sqrt_cycles,   dwt_us(results.sqrt_cycles));
        printf("  memcpy 256 B     : %5lu cy = %5.1f us\r\n",
               (unsigned long)results.memcpy_cycles,  dwt_us(results.memcpy_cycles));
        printf("  FIR 16tap/64smp  : %5lu cy = %5.1f us\r\n",
               (unsigned long)results.fir_cycles,     dwt_us(results.fir_cycles));
        printf("  sort 128 uint16  : %5lu cy = %5.1f us\r\n",
               (unsigned long)results.sort_cycles,    dwt_us(results.sort_cycles));
        printf("  dot-product 32f  : %5lu cy = %5.1f us\r\n",
               (unsigned long)results.dot_cycles,     dwt_us(results.dot_cycles));
        printf("  sem give→wake    : %5lu cy = %5.1f us\r\n",
               (unsigned long)results.sem_latency_cycles,
               dwt_us(results.sem_latency_cycles));
        printf("──────────────────────────────────────────────────\r\n\r\n");
        uart_unlock();
    }
}
