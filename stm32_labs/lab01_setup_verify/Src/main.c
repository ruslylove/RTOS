/*
 * Lab 01 — Toolchain & Environment Verify (WeAct STM32H503CBT6)
 * KMUTNB · M.Eng. Real-Time Operating Systems
 *
 * Two FreeRTOS tasks:
 *   vBlinkTask     (HIGH prio, 500 ms) — toggles PC13 LED + prints via UART
 *   vHeartbeatTask (LOW  prio, 1000 ms) — prints FreeRTOS tick uptime
 *
 * A shared UART mutex prevents garbled printf output.
 *
 * Build:  cmake --preset Debug && cmake --build build/Debug
 * Flash:  use VS Code ST-Link debug launch or STM32CubeProgrammer
 * Serial: PA9=TX, 115200 8N1 — connect a USB-UART adapter to a terminal
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "main.h"

/* ── RTOS objects ─────────────────────────────────────────────────────────── */
static SemaphoreHandle_t xUartMutex;

UART_HandleTypeDef huart1;

/* ── Forward declarations ─────────────────────────────────────────────────── */
static void vBlinkTask(void *pvParameters);
static void vHeartbeatTask(void *pvParameters);

/* ═══════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    HAL_ICACHE_Enable();
    uart_init();

    /* Enable LED GPIO */
    LED_CLK_EN();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
    LED_OFF();

    printf("\r\n=== RTOS Lab 01: Toolchain & Environment Verify ===\r\n");
    printf("Platform: WeAct STM32H503CBT6  |  CPU: 250 MHz Cortex-M33\r\n");
    printf("Tasks: Blink (500 ms) | Heartbeat (1000 ms)\r\n\r\n");

    xUartMutex = xSemaphoreCreateMutex();
    configASSERT(xUartMutex != NULL);

    xTaskCreate(vBlinkTask,     "Blink", 256, NULL, 2, NULL);
    xTaskCreate(vHeartbeatTask, "HB",    256, NULL, 1, NULL);

    printf("[MAIN] Starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    for (;;) ;
}

/* ── Blink task — toggles LED every 500 ms ──────────────────────────────── */
static void vBlinkTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t count = 0;

    for (;;) {
        LED_TOGGLE();
        count++;
        const char *state = (count & 1) ? "ON " : "OFF";

        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        printf("[BLINK] LED %s  (toggle #%lu)\r\n", state, (unsigned long)count);
        xSemaphoreGive(xUartMutex);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ── Heartbeat task — prints uptime every 1000 ms ───────────────────────── */
static void vHeartbeatTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        uint32_t uptime_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

        xSemaphoreTake(xUartMutex, portMAX_DELAY);
        printf("[HB]    uptime = %lu ms\r\n", (unsigned long)uptime_ms);
        xSemaphoreGive(xUartMutex);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ── FreeRTOS hooks ──────────────────────────────────────────────────────── */
void vApplicationTickHook(void)
{
    HAL_IncTick();          /* keep HAL tick counter alive under FreeRTOS */
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[FATAL] Stack overflow: %s\r\n", pcTaskName);
    for (;;) ;
}

void vApplicationMallocFailedHook(void)
{
    printf("[FATAL] FreeRTOS heap exhausted\r\n");
    for (;;) ;
}

/* ── printf retarget → USART1 ───────────────────────────────────────────── */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* ── Clock: HSI 64 MHz → PLL → 250 MHz ─────────────────────────────────── */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /* HSI 64 MHz, /16 → 4 MHz PLL input, ×125 → 500 MHz VCO, /2 → 250 MHz */
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSIDiv              = RCC_HSI_DIV1;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLL1_SOURCE_HSI;
    osc.PLL.PLLM            = 16;
    osc.PLL.PLLN            = 125;
    osc.PLL.PLLP            = 2;
    osc.PLL.PLLQ            = 2;
    osc.PLL.PLLR            = 2;
    osc.PLL.PLLFRACN        = 0;
    osc.PLL.PLLRGE          = RCC_PLL1_VCIRANGE_1;     /* 4-8 MHz VCI */
    osc.PLL.PLLVCOSEL       = RCC_PLL1_VCORANGE_WIDE;  /* 150-1500 MHz VCO */
    HAL_RCC_OscConfig(&osc);

    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                         RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    clk.APB3CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5);

    __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);
}

/* ── USART1 init: PA9=TX AF7, PA10=RX AF7, 115200 8N1 ──────────────────── */
void uart_init(void)
{
    UART_GPIO_CLK();
    UART_CLK_EN();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = UART_TX_PIN | UART_RX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = UART_GPIO_AF;
    HAL_GPIO_Init(UART_GPIO_PORT, &gpio);

    huart1.Instance          = UART_INSTANCE;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}
