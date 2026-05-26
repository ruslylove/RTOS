/*
 * Lab 01 — Toolchain & Environment Verification
 * KMUTNB · M.Eng. Real-Time Operating Systems
 *
 * Two FreeRTOS tasks confirm the toolchain, RTOS, and board BSP are all working.
 * Uses NXP MCUXpresso SDK: BOARD_InitHardware() and PRINTF from fsl_debug_console.
 *
 *   vBlinkTask     (HIGH prio, 500 ms)  — simulates LED blink over UART
 *   vHeartbeatTask (LOW  prio, 1000 ms) — prints uptime once per second
 *
 * Build:  make
 * Flash:  make flash
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"
#include "board.h"
#include "app.h"

/* ── Shared UART mutex ───────────────────────────────────────────────────── */
static SemaphoreHandle_t xUartMutex;

/* ── Tasks ───────────────────────────────────────────────────────────────── */

static void vBlinkTask(void *pvParams)
{
    (void)pvParams;
    uint32_t count = 0;

    for (;;) {
        count++;
        LED_RED_TOGGLE();
        const char *state = (count & 1u) ? "ON " : "OFF";

        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        PRINTF("[BLINK] LED %s  (toggle #%u)\r\n", state, (unsigned)count);
        xSemaphoreGive(xUartMutex);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void vHeartbeatTask(void *pvParams)
{
    (void)pvParams;

    for (;;) {
        uint32_t uptime_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        PRINTF("[HB]    uptime = %u ms\r\n", (unsigned)uptime_ms);
        xSemaphoreGive(xUartMutex);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ── Main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    BOARD_InitHardware();

    /* LED: enable PORT4 + GPIO4 clocks, configure PIO4_18 (RED LED) as output */
    CLOCK_EnableClock(kCLOCK_Port4);
    CLOCK_EnableClock(kCLOCK_Gpio4);
    const port_pin_config_t ledPortCfg = {
        kPORT_PullDisable, kPORT_LowPullResistor, kPORT_FastSlewRate,
        kPORT_PassiveFilterDisable, kPORT_OpenDrainDisable, kPORT_LowDriveStrength,
        kPORT_MuxAlt0, kPORT_InputBufferEnable, kPORT_InputNormal, kPORT_UnlockRegister
    };
    PORT_SetPinConfig(PORT4, BOARD_LED_RED_GPIO_PIN, &ledPortCfg);
    gpio_pin_config_t ledGpioCfg = { kGPIO_DigitalOutput, LOGIC_LED_OFF };
    GPIO_PinInit(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN, &ledGpioCfg);

    PRINTF("\r\n=== RTOS Lab 01: Toolchain & Environment Verify ===\r\n");
    PRINTF("SDK: BOARD_InitHardware + PRINTF  |  CPU: PLL 150 MHz\r\n");
    PRINTF("Tasks: Blink (500 ms) | Heartbeat (1000 ms)\r\n\r\n");

    xUartMutex = xSemaphoreCreateMutex();
    if (!xUartMutex) {
        PRINTF("FATAL: could not create mutex\r\n");
        for (;;) ;
    }

    xTaskCreate(vBlinkTask,     "Blink", 512, NULL, 2, NULL);
    xTaskCreate(vHeartbeatTask, "HB",    512, NULL, 1, NULL);

    PRINTF("[MAIN] Starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    for (;;) ;
}

/* ── FreeRTOS application hooks ─────────────────────────────────────────── */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    PRINTF("[FATAL] Stack overflow: %s\r\n", pcTaskName);
    for (;;) ;
}

void vApplicationMallocFailedHook(void)
{
    PRINTF("[FATAL] FreeRTOS heap exhausted\r\n");
    for (;;) ;
}
