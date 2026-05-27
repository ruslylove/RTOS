/*
 * overflow_demo.c
 * ─────────────────────────────────────────────────────────────────────────────
 * Lab 07, Part A — Stack Overflow Induction and Detection
 *
 * Step 1 (Part A): create vOverflowTask with 64 words (256 bytes) in main.c.
 *   configCHECK_FOR_STACK_OVERFLOW = 0
 *   Expected: HardFault or garbled output — no helpful diagnostic.
 *
 * Step 2 (Part A): set configCHECK_FOR_STACK_OVERFLOW = 2 in FreeRTOSConfig.h.
 *   Expected: vApplicationStackOverflowHook fires and prints the task name.
 *
 * Step 3 (Part B): increase stack to 512 words so the task runs safely.
 *   Run for 30 s, then read uxTaskGetStackHighWaterMark via vStackReportTask.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "FreeRTOS.h"
#include "task.h"
#include "overflow_demo.h"
#include "fsl_debug_console.h"

/* ── Recursive helper ────────────────────────────────────────────────────── */

/*
 * recursive_sum
 *
 * Each call frame allocates local_buffer[8] (32 bytes) plus the call overhead
 * (~32 bytes for saved registers on Cortex-M33) = ~64 bytes per level.
 * recursive_sum(60) requires ~3840 bytes of stack, far beyond 256 bytes.
 */
static uint32_t recursive_sum(uint32_t n)
{
    uint32_t local_buffer[8];
    int i;
    for (i = 0; i < 8; i++)
    {
        local_buffer[i] = (uint32_t)i + n;
    }
    if (n == 0u)
    {
        return 0u;
    }
    return local_buffer[0] + recursive_sum(n - 1u);
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* vOverflowTask                                                               */
/* ══════════════════════════════════════════════════════════════════════════ */

void vOverflowTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        PRINTF("[OVF] calling recursive_sum(60)...\r\n");

        /* This will overflow a 64-word (256-byte) stack */
        uint32_t result = recursive_sum(60u);

        /* If detection is off, execution may reach here with corrupt stack */
        PRINTF("[OVF] result = %lu  (should never print with 64-word stack)\r\n",
                    result);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* vApplicationStackOverflowHook                                              */
/*                                                                            */
/* Called by the FreeRTOS kernel at a context switch when                     */
/* configCHECK_FOR_STACK_OVERFLOW >= 1 and an overflow is detected.          */
/* ══════════════════════════════════════════════════════════════════════════ */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;

    /* Disable interrupts — we are in an unrecoverable state */
    taskDISABLE_INTERRUPTS();

    PRINTF("\r\n!!! STACK OVERFLOW in task: %s !!!\r\n", pcTaskName);

    /*
     * TODO (production system): log to non-volatile memory (e.g. Flash or
     * external EEPROM) before triggering a controlled reset, so you can
     * retrieve the diagnostic information after the reboot.
     *
     * Example:
     *   nvlog_write_crash_record(pcTaskName, xTaskGetTickCount());
     *   NVIC_SystemReset();
     */

    /* Halt here so the UART message stays visible on the terminal */
    for (;;) {}
}
