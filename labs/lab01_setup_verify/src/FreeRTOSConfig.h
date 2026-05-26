#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
    #include <stdint.h>
    extern uint32_t SystemCoreClock;
#endif

/* Clock / tick */
#define configCPU_CLOCK_HZ              (SystemCoreClock)
#define configTICK_RATE_HZ              1000UL

/* Scheduler */
#define configUSE_PREEMPTION            1
#define configUSE_TIME_SLICING          1
#define configUSE_TICKLESS_IDLE         0
#define configMAX_PRIORITIES            5
#define configMINIMAL_STACK_SIZE        128
#define configMAX_TASK_NAME_LEN         20
#define configIDLE_SHOULD_YIELD         1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

/* Memory */
#define configTOTAL_HEAP_SIZE           (12 * 1024)
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION  0
#define configFRTOS_MEMORY_SCHEME        4

/* Software timers — not used but timers.c is compiled */
#define configUSE_TIMERS                1
#define configTIMER_TASK_PRIORITY       (configMAX_PRIORITIES - 1)
#define configTIMER_TASK_STACK_DEPTH    (configMINIMAL_STACK_SIZE * 2)
#define configTIMER_QUEUE_LENGTH        2

/* Synchronisation */
#define configUSE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_RECURSIVE_MUTEXES     1
#define configUSE_TASK_NOTIFICATIONS    1
#define configUSE_EVENT_GROUPS          1
#define configUSE_STREAM_BUFFERS        1
#define configQUEUE_REGISTRY_SIZE       8

/* Hooks */
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configCHECK_FOR_STACK_OVERFLOW  2
#define configUSE_MALLOC_FAILED_HOOK    1

/* Run-time stats */
#define configGENERATE_RUN_TIME_STATS   0

/* Trace / debug */
#define configUSE_TRACE_FACILITY        1
#define configRECORD_STACK_HIGH_ADDRESS 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1
#define configENABLE_BACKWARD_COMPATIBILITY  1

/* NXP additions header (needed by SDK freertos-kernel-upstream) */
#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H 1

/* ARM_CM33_NTZ port — MCXN236 boots in secure world, FreeRTOS runs entirely
 * in secure mode.  configRUN_FREERTOS_SECURE_ONLY=1 sets EXC_RETURN bit[6]=1
 * (secure exception) so vRestoreContextOfFirstTask succeeds.
 * Without this, the port uses EXC_RETURN 0xFFFFFFBC (non-secure bit[6]=0)
 * which causes an immediate fault when returning from the first SVC. */
#define configRUN_FREERTOS_SECURE_ONLY  1
#define configENABLE_FPU                1
#define configENABLE_MPU                0
#define configENABLE_TRUSTZONE          0
#define configTICK_TYPE_WIDTH_IN_BITS   TICK_TYPE_WIDTH_32_BITS

/* Cortex-M33 interrupt priorities (3-bit NVIC, matches SDK configPRIO_BITS=3) */
#define configPRIO_BITS                 3
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         ((1U << (configPRIO_BITS)) - 1U)
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    2
#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_API_CALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Check handler installation at startup */
#define configCHECK_HANDLER_INSTALLATION 1

/* Assert */
#define configASSERT(x) if ((x) == 0) { taskDISABLE_INTERRUPTS(); for (;;); }

/* Map FreeRTOS port handlers to CMSIS vector names */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/* API subsets */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_uxTaskGetStackHighWaterMark2    1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xSemaphoreGetMutexHolder        1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_xTaskAbortDelay                 1

#endif /* FREERTOS_CONFIG_H */
