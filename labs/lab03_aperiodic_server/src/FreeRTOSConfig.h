#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* MCXN236 at 150 MHz (PLL configured in hw_init). */
#define configCPU_CLOCK_HZ              150000000UL
#define configTICK_RATE_HZ              1000UL      /* 1 ms tick */

/* Scheduler */
#define configUSE_PREEMPTION            1
#define configUSE_TIME_SLICING          1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE         0
#define configMAX_PRIORITIES            6
#define configMINIMAL_STACK_SIZE        256         /* words */
#define configMAX_TASK_NAME_LEN         16

/* Memory */
#define configTOTAL_HEAP_SIZE           (64 * 1024) /* 64 KB */
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION  0

/* Software timers */
#define configUSE_TIMERS                1
#define configTIMER_TASK_PRIORITY       (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH        8
#define configTIMER_TASK_STACK_DEPTH    256

/* Synchronisation primitives */
#define configUSE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_RECURSIVE_MUTEXES     0

/* Event groups */
#define configUSE_EVENT_GROUPS          1

/* Hooks */
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configCHECK_FOR_STACK_OVERFLOW  2
#define configUSE_MALLOC_FAILED_HOOK    1

/* Trace / stats */
#define configUSE_TRACE_FACILITY        1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1
#define configGENERATE_RUN_TIME_STATS   0

/* ARM Cortex-M33 port */
#define configTICK_TYPE_WIDTH_IN_BITS   TICK_TYPE_WIDTH_32_BITS
#define configENABLE_FPU                0
#define configENABLE_MPU                0
#define configENABLE_TRUSTZONE          0

/* Interrupt priority levels (4-bit field on MCXN236) */
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
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_vTaskDelayUntil             1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskPrioritySet            1
#define INCLUDE_uxTaskPriorityGet           1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetSchedulerState      1
#define INCLUDE_xTaskGetTickCount           1

/*
 * Utilisation calculation (Polling Server treated as periodic task):
 *   tau1: C=5ms  T=100ms  U=0.050
 *   tau2: C=5ms  T=200ms  U=0.025
 *   PS  : C=10ms T=50ms   U=0.200   (budget=Bs, period=Ts)
 *   Total U = 0.275  <  ln(2) = 0.693  -> schedulable under RMS
 */

#endif /* FREERTOS_CONFIG_H */
