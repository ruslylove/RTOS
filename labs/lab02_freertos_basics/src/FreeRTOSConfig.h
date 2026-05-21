#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* MCXN236 boots from FRO 12 MHz; upgrade to 96 MHz is done in hw_init(). */
#define configCPU_CLOCK_HZ              96000000UL
#define configTICK_RATE_HZ              1000UL      /* 1 ms tick */

/* Scheduler */
#define configUSE_PREEMPTION            1
#define configUSE_TIME_SLICING          1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE         0
#define configMAX_PRIORITIES            5
#define configMINIMAL_STACK_SIZE        256         /* in words */
#define configMAX_TASK_NAME_LEN         12

/* Memory */
#define configTOTAL_HEAP_SIZE           (20 * 1024) /* 20 KB for FreeRTOS heap */
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION  0

/* Timers */
#define configUSE_TIMERS                1
#define configTIMER_TASK_PRIORITY       (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH        8
#define configTIMER_TASK_STACK_DEPTH    256

/* Queue / semaphore */
#define configUSE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_RECURSIVE_MUTEXES     0

/* Hooks */
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configCHECK_FOR_STACK_OVERFLOW  2
#define configUSE_MALLOC_FAILED_HOOK    1

/* Run-time stats (requires a free-running counter; disable for now) */
#define configGENERATE_RUN_TIME_STATS   0

/* Trace */
#define configUSE_TRACE_FACILITY        1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1

/* ARM_CM33_NTZ port requirements */
#define configTICK_TYPE_WIDTH_IN_BITS   TICK_TYPE_WIDTH_32_BITS
#define configENABLE_FPU                0
#define configENABLE_MPU                0
#define configENABLE_TRUSTZONE          0

/* Cortex-M33 interrupt priorities.
 * Bits [7:4] are implemented (4-bit priority field on MCXN236).
 * configMAX_SYSCALL_INTERRUPT_PRIORITY sets the highest priority ISR that
 * may call FreeRTOS API functions ending in "FromISR". */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - 4))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - 4))

/* Map FreeRTOS port handlers to CMSIS vector names used in startup file */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/* API subsets */
#define INCLUDE_vTaskDelay              1
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskDelete             1
#define INCLUDE_vTaskPrioritySet        1
#define INCLUDE_uxTaskPriorityGet       1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetSchedulerState  1

#endif /* FREERTOS_CONFIG_H */
