#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#include <stdint.h>
/* ARM_CM33_NTZ port mandatory defines */
#define configENABLE_FPU          1   /* STM32H503 has Cortex-M33 FPU (SP_FPU) */
#define configENABLE_MPU          0   /* not using the FreeRTOS MPU port        */
#define configENABLE_TRUSTZONE    0   /* STM32H503 NO_TZ configuration          */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

/*
 * Tickless idle: 1 = use the port-supplied implementation.
 * The ARM CM33 NTZ port stops SysTick, executes WFI (sleep until next IRQ),
 * then corrects the tick count on wakeup.
 * Change to 0 to compare active-polling idle vs tickless.
 */
#define configUSE_TICKLESS_IDLE                 1

#define configCPU_CLOCK_HZ                      250000000UL
#define configTICK_RATE_HZ                      1000U

/*
 * Minimum idle ticks before entering tickless sleep.
 * Default 2; increase to reduce wakeup frequency at the cost of latency.
 */
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP   2

#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                256
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configTOTAL_HEAP_SIZE                   (20 * 1024)
#define configUSE_IDLE_HOOK                     1   /* vApplicationIdleHook â€” counts idle entries */
#define configUSE_TICK_HOOK                     1   /* vApplicationTickHook â€” HAL_IncTick()       */
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                8
#define configTIMER_TASK_STACK_DEPTH            256
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY     5
#define configKERNEL_INTERRUPT_PRIORITY   (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define configASSERT(x) do { if (!(x)) { taskDISABLE_INTERRUPTS(); for (;;); } } while (0)
#endif
