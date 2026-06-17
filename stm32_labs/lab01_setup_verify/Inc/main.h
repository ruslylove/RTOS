#ifndef MAIN_H
#define MAIN_H

#include "stm32h5xx_hal.h"

/* WeAct STM32H503CBT6 — onboard LED on PC13, active-low */
#define LED_PIN         GPIO_PIN_13
#define LED_PORT        GPIOC
#define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()
#define LED_ON()        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET)
#define LED_OFF()       HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)
#define LED_TOGGLE()    HAL_GPIO_TogglePin(LED_PORT, LED_PIN)

/* USART1 on PA9 (TX) / PA10 (RX) — connect USB-UART adapter or use SWO */
#define UART_INSTANCE   USART1
#define UART_TX_PIN     GPIO_PIN_9
#define UART_RX_PIN     GPIO_PIN_10
#define UART_GPIO_PORT  GPIOA
#define UART_GPIO_AF    GPIO_AF7_USART1
#define UART_CLK_EN()   __HAL_RCC_USART1_CLK_ENABLE()
#define UART_GPIO_CLK() __HAL_RCC_GPIOA_CLK_ENABLE()

extern UART_HandleTypeDef huart1;

void SystemClock_Config(void);
void uart_init(void);

#endif /* MAIN_H */
