/*
 * Lab 08 — MPU Memory Protection (WeAct STM32H503CBT6)
 *
 * Demonstrates the Cortex-M33 ARM v8-M MPU:
 *   - secure_buffer declared 32-byte aligned; MPU Region 0 marks it Read-Only
 *   - vNormalTask reads the buffer (allowed)
 *   - vRogueTask (USE_ROGUE_TASK=1) writes to it → MemManage_Handler fires
 *   - MemManage_Handler decodes MMFSR / MMFAR to identify the violation
 */

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stm32h5xx_hal.h"
#include "mpu_config.h"
#include "mpu_tasks.h"

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

    /* Configure MPU before starting the scheduler */
    mpu_init();

    printf("\r\n=== RTOS Lab 08: MPU Memory Protection ===\r\n");
    printf("Cortex-M33 ARM v8-M MPU — %lu regions available\r\n\r\n",
           (unsigned long)((MPU->TYPE >> 8) & 0xFFU));

    mpu_dump_config();

    xUartMutex = xSemaphoreCreateMutex();
    configASSERT(xUartMutex != NULL);

    xTaskCreate(vNormalTask,  "Normal",  512, NULL, 2, NULL);
    xTaskCreate(vMonitorTask, "Monitor", 512, NULL, 1, NULL);

#if USE_ROGUE_TASK
    xTaskCreate(vRogueTask,   "Rogue",   512, NULL, 3, NULL);
    printf("[main] USE_ROGUE_TASK=1 — MemManage fault expected in ~3 s\r\n\r\n");
#else
    printf("[main] USE_ROGUE_TASK=0 — normal read-only demo mode\r\n");
    printf("       Set USE_ROGUE_TASK 1 in mpu_tasks.h to trigger the fault.\r\n\r\n");
#endif

    vTaskStartScheduler();
    for (;;) ;
}

/* ── MemManage fault handler ────────────────────────────────────────────── */
void MemManage_Handler(void)
{
    /*
     * SCB->CFSR bits [7:0] = MMFSR (MemManage Fault Status Register)
     *   [7] MMARVALID — SCB->MMFAR holds the fault address
     *   [4] MSTKERR   — fault on exception entry stacking
     *   [3] MUNSTKERR — fault on exception return unstacking
     *   [1] DACCVIOL  — data access violation (write to RO region)
     *   [0] IACCVIOL  — instruction access violation (execute from NX region)
     */
    volatile uint32_t mmfsr = SCB->CFSR & 0xFFU;
    volatile uint32_t mmfar = SCB->MMFAR;

    static char msg[128];
    int len;

    len = snprintf(msg, sizeof(msg),
        "\r\n[MPU FAULT] MMFSR=0x%02lX  MMFAR=0x%08lX\r\n"
        "  DACCVIOL=%lu  IACCVIOL=%lu  MMARVALID=%lu\r\n"
        "  => Write attempt at 0x%08lX blocked by MPU Region 0\r\n",
        (unsigned long)mmfsr,
        (unsigned long)mmfar,
        (unsigned long)((mmfsr >> 1) & 1U),
        (unsigned long)((mmfsr >> 0) & 1U),
        (unsigned long)((mmfsr >> 7) & 1U),
        (unsigned long)mmfar);

    HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)len, 300);
    for (;;) ;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    const char *msg = "\r\n[FATAL] Stack overflow: ";
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, 25, 100);
    HAL_UART_Transmit(&huart1, (uint8_t *)pcTaskName, 16, 100);
    HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n", 2, 100);
    for (;;) ;
}

void vApplicationTickHook(void)      { HAL_IncTick(); }
void vApplicationMallocFailedHook(void)
    { printf("[FATAL] Heap alloc failed\r\n"); for (;;); }

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
