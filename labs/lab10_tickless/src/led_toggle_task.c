/*
 * led_toggle_task.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 10 — Tickless Idle: LED Toggle Task
 *
 * Toggles the onboard LED every 1 s and prints the tick count so timing
 * accuracy can be compared between Part A (configUSE_TICKLESS_IDLE = 0)
 * and Part B (configUSE_TICKLESS_IDLE = 1).
 *
 * GPIO pin assignment (FRDM-MCXN236 green LED):
 *   GPIO0, pin 10 — adjust LED_GPIO / LED_PIN for your board revision.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "led_toggle_task.h"

/* LED period */
#define LED_PERIOD_MS  1000U

/*
 * FRDM-MCXN236 green LED: GPIO0, pin 10.
 * TODO: verify against your board's schematic / pin_mux.c.
 */
#define LED_GPIO   GPIO0
#define LED_PIN    10U

/* LED state */
static uint8_t s_led_on = 0U;

static void led_init(void)
{
    gpio_pin_config_t cfg = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic  = 1U,   /* start OFF (active-low) */
    };
    GPIO_PinInit(LED_GPIO, LED_PIN, &cfg);
}

static void led_toggle(void)
{
    s_led_on ^= 1U;
    GPIO_PinWrite(LED_GPIO, LED_PIN, s_led_on ? 0U : 1U);  /* active-low */
}

void vLEDTask(void *pvParameters)
{
    (void)pvParameters;

    led_init();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(LED_PERIOD_MS);
    uint32_t toggle_count = 0;

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        led_toggle();
        toggle_count++;

        PRINTF("[LED] toggle #%lu  tick=%lu  state=%s\r\n",
               (unsigned long)toggle_count,
               (unsigned long)xTaskGetTickCount(),
               s_led_on ? "ON" : "OFF");
    }
}
