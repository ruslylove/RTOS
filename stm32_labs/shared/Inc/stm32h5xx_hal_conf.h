/* Minimal HAL configuration for stm32_labs — enables only what the labs use. */

#ifndef STM32H5xx_HAL_CONF_H
#define STM32H5xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_ICACHE_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED      /* Lab 05 watchdog */
#define HAL_LPTIM_MODULE_ENABLED     /* Lab 10 tickless */
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED       /* Lab 06 WCET baseline */
#define HAL_UART_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED       /* Lab 10 */

/* HSE crystal — set to 0 if your WeAct board has no external crystal */
#define HSE_VALUE    24000000U
#define HSE_STARTUP_TIMEOUT    100U
#define LSE_VALUE    32768U
#define LSE_STARTUP_TIMEOUT   5000U
#define HSI_VALUE        64000000U
#define HSI48_VALUE      48000000U   /* HSI48 internal oscillator (USB/RNG clock) */
#define CSI_VALUE         4000000U
#define LSI_VALUE           32000U
#define EXTERNAL_CLOCK_VALUE  12288000U  /* I2S/SAI external clock — set to 0 if unused */

#define VDD_VALUE              3300U
#define TICK_INT_PRIORITY      15U
#define USE_RTOS               0U    /* must be 0 — STM32H5 HAL does not support USE_RTOS=1 */
#define PREFETCH_ENABLE        1U

/* assert_param: disabled (no USE_FULL_ASSERT); HAL uses this internally */
#define assert_param(expr)     ((void)0U)

/* Include HAL module headers */
#include "stm32h5xx_hal_rcc.h"
#include "stm32h5xx_hal_rcc_ex.h"
#include "stm32h5xx_hal_gpio.h"
#include "stm32h5xx_hal_dma.h"
#include "stm32h5xx_hal_dma_ex.h"
#include "stm32h5xx_hal_cortex.h"
#include "stm32h5xx_hal_flash.h"
#include "stm32h5xx_hal_flash_ex.h"
#include "stm32h5xx_hal_pwr.h"
#include "stm32h5xx_hal_pwr_ex.h"
#include "stm32h5xx_hal_exti.h"
#include "stm32h5xx_hal_uart.h"
#include "stm32h5xx_hal_uart_ex.h"
#include "stm32h5xx_hal_iwdg.h"
#include "stm32h5xx_hal_lptim.h"
#include "stm32h5xx_hal_icache.h"
#include "stm32h5xx_hal_tim.h"
#include "stm32h5xx_hal_tim_ex.h"
#include "stm32h5xx_hal_adc.h"
#include "stm32h5xx_hal_adc_ex.h"

#ifdef __cplusplus
}
#endif

#endif /* STM32H5xx_HAL_CONF_H */
