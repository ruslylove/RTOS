/*
 * Lab 03 — Aperiodic Servers & Producer-Consumer Pipeline
 * WeAct STM32H503CBT6
 *
 * Enable one section at a time by uncommenting:
 *   ENABLE_PART_A — deferred interrupt handler (semaphore)
 *   ENABLE_PART_B — polling server (queue-based)
 *   ENABLE_PART_C — sensor pipeline (queue + event groups)
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"
#include "stm32h5xx_hal.h"
#include "periodic_tasks.h"
#include "aperiodic_handler.h"
#include "producer_consumer.h"

#define ENABLE_PERIODIC_TASKS  1
#define ENABLE_PART_A          1
/* #define ENABLE_PART_B       1 */
/* #define ENABLE_PART_C       1 */

/* ── Shared UART mutex (used by all task files via weak linkage) ─────────── */
static SemaphoreHandle_t xUartMutex;

void uart_lock(void)   { xSemaphoreTake(xUartMutex, portMAX_DELAY); }
void uart_unlock(void) { xSemaphoreGive(xUartMutex); }

/* ── Hardware ──────────────────────────────────────────────────────────────── */
static UART_HandleTypeDef huart1;
static void SystemClock_Config(void);
static void uart_board_init(void);

/* ── Aperiodic event simulator (fires every ~1500 ms via software timer) ──── */
#ifdef ENABLE_PART_A
static void vEventSimTimer(TimerHandle_t t)
{
    (void)t;
    vSimulateAperiodicEvent();
}
#endif

/* ═══════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    HAL_ICACHE_Enable();
    uart_board_init();

    printf("\r\n=== RTOS Lab 03: Aperiodic Servers & Producer-Consumer ===\r\n\r\n");

    xUartMutex = xSemaphoreCreateMutex();
    configASSERT(xUartMutex != NULL);

#ifdef ENABLE_PERIODIC_TASKS
    xTaskCreate(vPeriodic1Task, "P1", 256, NULL, 3, NULL);
    xTaskCreate(vPeriodic2Task, "P2", 256, NULL, 2, NULL);
#endif

#ifdef ENABLE_PART_A
    xAperiodicSem = xSemaphoreCreateBinary();
    configASSERT(xAperiodicSem != NULL);
    xTaskCreate(vAperiodicHandlerTask, "AperH", 256, NULL, 5, NULL);
    /* Simulate aperiodic events every 1500 ms */
    TimerHandle_t xEvSim = xTimerCreate("EvSim", pdMS_TO_TICKS(1500),
                                        pdTRUE, NULL, vEventSimTimer);
    xTimerStart(xEvSim, 0);
#endif

#ifdef ENABLE_PART_B
    xRequestQueue = xQueueCreate(8, sizeof(AperiodicRequest_t));
    configASSERT(xRequestQueue != NULL);
    xTaskCreate(vPollingServerTask, "PS", 256, NULL, 3, NULL);
    /* Simulate events via Part A's timer callback (also fills xRequestQueue) */
    xAperiodicSem = xSemaphoreCreateBinary();
    TimerHandle_t xEvSim2 = xTimerCreate("EvSim2", pdMS_TO_TICKS(1200),
                                         pdTRUE, NULL, vEventSimTimer);
    xTimerStart(xEvSim2, 0);
#endif

#ifdef ENABLE_PART_C
    xPipelineQueue  = xQueueCreate(16, sizeof(SensorReading_t));
    xPipelineEvents = xEventGroupCreate();
    configASSERT(xPipelineQueue && xPipelineEvents);
    xTaskCreate(vSensor1Task, "S1",     256, NULL, 3, NULL);
    xTaskCreate(vSensor2Task, "S2",     256, NULL, 2, NULL);
    xTaskCreate(vLoggerTask,  "Logger", 512, NULL, 1, NULL);
    xTaskCreate(vMonitorTask, "Mon",    256, NULL, 2, NULL);
#endif

    printf("[MAIN] free heap before scheduler: %lu B\r\n",
           (unsigned long)xPortGetFreeHeapSize());
    vTaskStartScheduler();
    for (;;) ;
}

void vApplicationTickHook(void)      { HAL_IncTick(); }
void vApplicationStackOverflowHook(TaskHandle_t t, char *n)
    { (void)t; printf("[FATAL] Stack: %s\r\n", n); for (;;); }
void vApplicationMallocFailedHook(void)
    { printf("[FATAL] Heap\r\n"); for (;;); }

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0}; RCC_ClkInitTypeDef clk = {0};
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI; osc.HSIState = RCC_HSI_ON;
    osc.HSIDiv = RCC_HSI_DIV1; osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_ON; osc.PLL.PLLSource = RCC_PLL1_SOURCE_HSI;
    osc.PLL.PLLM = 16; osc.PLL.PLLN = 125; osc.PLL.PLLP = 2;
    osc.PLL.PLLQ = 2; osc.PLL.PLLR = 2; osc.PLL.PLLFRACN = 0;
    osc.PLL.PLLRGE = RCC_PLL1_VCIRANGE_1; osc.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                    RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = clk.APB1CLKDivider = clk.APB2CLKDivider = clk.APB3CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5);
    __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);
}

static void uart_board_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_USART1_CLK_ENABLE();
    GPIO_InitTypeDef gpio = { .Pin = GPIO_PIN_9 | GPIO_PIN_10,
                               .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL,
                               .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF7_USART1 };
    HAL_GPIO_Init(GPIOA, &gpio);
    huart1.Instance = USART1; huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B; huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE; huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE; huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}
