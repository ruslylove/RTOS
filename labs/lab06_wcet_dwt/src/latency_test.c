/*
 * latency_test.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 06, Part B — GPIO Interrupt Latency Measurement
 *
 * ISR reads the DWT counter as its first instruction, then toggles P1_2.
 * The latency (cycles from gpio assertion to ISR entry) is stored in
 * latency_buffer[] for post-collection analysis.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "latency_test.h"
#include "dwt.h"
#include "uart.h"

/* NXP SDK GPIO driver */
#include "fsl_gpio.h"

/* ── Shared latency data ──────────────────────────────────────────────────── */
volatile uint32_t latency_buffer[LATENCY_BUF_SIZE];
volatile uint32_t latency_idx     = 0;
volatile uint32_t last_gpio_toggle = 0;

/* ── GPIO pin assignments ─────────────────────────────────────────────────── */
#define TEST_PORT     GPIO1
#define PIN_OUTPUT    0u    /* P1_0: toggle source */
#define PIN_INPUT     1u    /* P1_1: interrupt input (wire to P1_0) */
#define PIN_PROBE     2u    /* P1_2: ISR response (scope probe) */

/* ══════════════════════════════════════════════════════════════════════════ */
/* One-time hardware setup                                                    */
/* ══════════════════════════════════════════════════════════════════════════ */

void latency_test_init(void)
{
    /* TODO: configure pin MUX to GPIO function for P1_0, P1_1, P1_2
     * using IOCON / PORT peripheral registers or the SDK's PORT_SetPinMux().
     * Example (MCUXpresso SDK):
     *   PORT_SetPinMux(PORT1, PIN_OUTPUT, kPORT_MuxAsGpio);
     *   PORT_SetPinMux(PORT1, PIN_INPUT,  kPORT_MuxAsGpio);
     *   PORT_SetPinMux(PORT1, PIN_PROBE,  kPORT_MuxAsGpio);
     */

    /* Configure P1_1 as input, rising-edge interrupt */
    gpio_pin_config_t in_cfg = {kGPIO_DigitalInput, 0, kGPIO_IntRisingEdge};
    GPIO_PinInit(TEST_PORT, PIN_INPUT, &in_cfg);
    GPIO_EnableInterrupts(TEST_PORT, 1u << PIN_INPUT);

    /* Configure P1_0 and P1_2 as outputs, initially low */
    gpio_pin_config_t out_cfg = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
    GPIO_PinInit(TEST_PORT, PIN_OUTPUT, &out_cfg);
    GPIO_PinInit(TEST_PORT, PIN_PROBE,  &out_cfg);

    /* TODO: Set NVIC priority and enable the GPIO1 interrupt.
     * Priority must be >= configMAX_SYSCALL_INTERRUPT_PRIORITY to allow
     * FromISR API calls (or lower number = higher priority on ARM).
     *
     * NVIC_SetPriority(GPIO1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
     * EnableIRQ(GPIO1_IRQn);
     */
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* GPIO1 ISR — reads DWT as first instruction                                 */
/* ══════════════════════════════════════════════════════════════════════════ */

void GPIO1_IRQHandler(void)
{
    /* Capture cycle count at ISR entry — must be the very first statement */
    uint32_t t_isr = DWT_GetCycles();

    GPIO_ClearPinsInterruptFlags(TEST_PORT, 1u << PIN_INPUT);

    /* Toggle the probe pin so latency is visible on the oscilloscope */
    GPIO_TogglePinsOutput(TEST_PORT, 1u << PIN_PROBE);

    /* Record latency into the ring buffer */
    uint32_t idx = latency_idx % LATENCY_BUF_SIZE;
    latency_buffer[idx] = t_isr - last_gpio_toggle;
    latency_idx++;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Task: generates the 1 kHz test signal on P1_0                              */
/* ══════════════════════════════════════════════════════════════════════════ */

void vGpioToggleTask(void *pvParameters)
{
    (void)pvParameters;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));

        /* Record DWT just before asserting the pin — ISR measures from here */
        last_gpio_toggle = DWT_GetCycles();
        GPIO_SetPinsOutput(TEST_PORT, 1u << PIN_OUTPUT);

        /* Brief high pulse — clear the pin after one tick */
        vTaskDelay(1);
        GPIO_ClearPinsOutput(TEST_PORT, 1u << PIN_OUTPUT);
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Task: holds a FreeRTOS critical section to demonstrate latency impact      */
/* ══════════════════════════════════════════════════════════════════════════ */

void vCriticalSectionTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /* Enter a ~1 ms critical section — ISRs at or below
         * configMAX_SYSCALL_INTERRUPT_PRIORITY are masked during this time. */
        taskENTER_CRITICAL();
        volatile uint32_t i = 0;
        while (i++ < 150000u) {}   /* ~1 ms busy-wait at 150 MHz */
        taskEXIT_CRITICAL();

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* Task: prints collected latency samples                                      */
/* ══════════════════════════════════════════════════════════════════════════ */

void vLatencyReportTask(void *pvParameters)
{
    (void)pvParameters;

    /* Wait for the buffer to fill */
    while (latency_idx < LATENCY_BUF_SIZE)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* TODO: compute best/worst/average from latency_buffer[] and print a
     * summary table. Also convert cycles to nanoseconds using
     * DWT_CYCLES_TO_NS(). */
    uint32_t best = UINT32_MAX, worst = 0, total = 0;

    for (int i = 0; i < LATENCY_BUF_SIZE; i++)
    {
        uint32_t v = latency_buffer[i];
        if (v < best)  best  = v;
        if (v > worst) worst = v;
        total += v;
    }

    uart_printf("\r\n=== Interrupt Latency Report ===\r\n");
    uart_printf("  Samples : %u\r\n",  LATENCY_BUF_SIZE);
    uart_printf("  Best    : %lu cycles  (%.1f ns)\r\n",
                best,  DWT_CYCLES_TO_NS(best));
    uart_printf("  Worst   : %lu cycles  (%.1f ns)\r\n",
                worst, DWT_CYCLES_TO_NS(worst));
    uart_printf("  Average : %lu cycles  (%.1f ns)\r\n",
                total / LATENCY_BUF_SIZE,
                DWT_CYCLES_TO_NS(total / LATENCY_BUF_SIZE));

    vTaskDelete(NULL);
}
