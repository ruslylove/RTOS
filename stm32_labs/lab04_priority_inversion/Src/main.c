/*
 * Lab 04 — Priority Inversion (WeAct STM32H503CBT6)
 *
 * Demonstrates unbounded priority inversion using a binary semaphore,
 * then resolves it by switching to a priority-inheriting FreeRTOS mutex.
 *
 * STEP 1: Leave USE_MUTEX undefined in inversion_tasks.h → observe HIGH blocked by MED.
 * STEP 2: Define USE_MUTEX                               → HIGH unblocks quickly.
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stm32h5xx_hal.h"
#include "inversion_tasks.h"

static UART_HandleTypeDef huart1;
static SemaphoreHandle_t xUartMutex;

void uart_lock(void)   { xSemaphoreTake(xUartMutex, portMAX_DELAY); }
void uart_unlock(void) { xSemaphoreGive(xUartMutex); }

static void SystemClock_Config(void);
static void uart_init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    HAL_ICACHE_Enable();
    uart_init();

    printf("\r\n=== RTOS Lab 04: Priority Inversion ===\r\n");
#ifdef USE_MUTEX
    printf("Guard type: MUTEX (priority inheritance enabled)\r\n\r\n");
#else
    printf("Guard type: BINARY SEMAPHORE (inversion will occur)\r\n\r\n");
#endif

    xUartMutex = xSemaphoreCreateMutex();

#ifdef USE_MUTEX
    xSharedResource = xSemaphoreCreateMutex();
#else
    xSharedResource = xSemaphoreCreateBinary();
    xSemaphoreGive(xSharedResource); /* binary sem starts unavailable */
#endif
    configASSERT(xSharedResource != NULL);

    xTaskCreate(vHighPrioTask, "HIGH", 256, NULL, 5, NULL);
    xTaskCreate(vMedPrioTask,  "MED",  256, NULL, 3, NULL);
    xTaskCreate(vLowPrioTask,  "LOW",  256, NULL, 1, NULL);

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

static void uart_init(void)
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
