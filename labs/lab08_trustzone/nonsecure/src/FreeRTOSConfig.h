/*
 * FreeRTOSConfig.h -- Non-Secure FreeRTOS configuration
 *
 * TrustZone-M specific settings:
 *   configENABLE_TRUSTZONE  = 1  -- kernel allocates a Secure context per task
 *   configRUN_FREERTOS_SECURE_ONLY = 0 -- NS kernel, NS tasks
 *
 * Lab 08 -- TrustZone-M: Secure/Non-Secure Partition
 * Platform: NXP FRDM-MCXN236 (Cortex-M33 @ 150 MHz)
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ── Core configuration ──────────────────────────────────────────────────── */
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      150000000UL
#define configTICK_RATE_HZ                      1000
#define configMAX_PRIORITIES                    7
#define configMINIMAL_STACK_SIZE                256
#define configTOTAL_HEAP_SIZE                   ( 32 * 1024 )
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_TRACE_FACILITY                0
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configQUEUE_REGISTRY_SIZE               8
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_APPLICATION_TASK_TAG          0
#define configUSE_COUNTING_SEMAPHORES           1

/* ── TrustZone-M support ─────────────────────────────────────────────────── */
/*
 * configENABLE_TRUSTZONE = 1
 *   The FreeRTOS Cortex-M33 port allocates a Secure context for each NS task
 *   that makes NSC calls.  The NS kernel saves/restores PSPLIM and the Secure
 *   stack pointer on every context switch involving such tasks.
 */
#define configENABLE_TRUSTZONE                  1

/*
 * configRUN_FREERTOS_SECURE_ONLY = 0
 *   The kernel runs in Non-Secure state.  NS tasks may call Secure functions
 *   via NSC veneers.
 */
#define configRUN_FREERTOS_SECURE_ONLY          0

/* Secure context size per NS task (words).  Must match the Secure-side
 * stack configured for secure function calls. */
#define configSECURE_CONTEXT_SIZE               256

/* ── Software timer ──────────────────────────────────────────────────────── */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            256

/* ── API inclusions ──────────────────────────────────────────────────────── */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetSchedulerState          1

/* ── Cortex-M33 interrupt priority configuration ─────────────────────────── */
#define configPRIO_BITS                         3
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         0x07
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    0x01
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Map FreeRTOS port exception handlers */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
