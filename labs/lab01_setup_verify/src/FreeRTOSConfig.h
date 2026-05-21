#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* MCXN236: FRO 12 MHz on boot; hw_init() switches to FRO 96 MHz. */
#define configCPU_CLOCK_HZ              96000000UL
#define configTICK_RATE_HZ              1000UL      /* 1 ms tick */

/* Scheduler */
#define configUSE_PREEMPTION            1
#define configUSE_TIME_SLICING          1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE         0
#define configMAX_PRIORITIES            5
#define configMINIMAL_STACK_SIZE        128         /* words — smaller than lab02 */
#define configMAX_TASK_NAME_LEN         12

/* Memory */
#define configTOTAL_HEAP_SIZE           (12 * 1024) /* 12 KB — lab01 needs less */
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION  0

/* Software timers — not used in lab01 */
#define configUSE_TIMERS                0

/* Synchronisation */
#define configUSE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES   0
#define configUSE_RECURSIVE_MUTEXES     0

/* Hooks */
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configCHECK_FOR_STACK_OVERFLOW  2
#define configUSE_MALLOC_FAILED_HOOK    1

/* Run-time stats */
#define configGENERATE_RUN_TIME_STATS   0

/* Trace — required by SystemView */
#define configUSE_TRACE_FACILITY        1
#define configRECORD_STACK_HIGH_ADDRESS 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1

/* ARM_CM33_NTZ port */
#define configTICK_TYPE_WIDTH_IN_BITS   TICK_TYPE_WIDTH_32_BITS
#define configENABLE_FPU                0
#define configENABLE_MPU                0
#define configENABLE_TRUSTZONE          0

/* Cortex-M33 interrupt priorities (4-bit field on MCXN236) */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - 4))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - 4))

/* Map FreeRTOS port handlers to CMSIS vector names */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/* API subsets */
#define INCLUDE_vTaskDelay              1
#define INCLUDE_vTaskDelete             1
#define INCLUDE_vTaskPrioritySet        1
#define INCLUDE_uxTaskPriorityGet       1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetSchedulerState  1

/* SystemView hook — included only when building with SYSVIEW=1 */
#ifdef USE_SYSVIEW
#include "SEGGER_SYSVIEW_FreeRTOS.h"
#endif

#endif /* FREERTOS_CONFIG_H */
