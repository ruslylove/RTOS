/*
 * secure_api.c -- Secure API implementation (NSC entry functions)
 *
 * Each function decorated with __attribute__((cmse_nonsecure_entry)) is
 * placed in the .gnu.sgstubs section by the linker and becomes callable
 * from Non-Secure world via a Secure Gateway (SG) instruction veneer.
 *
 * Build with:
 *   arm-none-eabi-gcc -mcmse ...
 *   arm-none-eabi-ld  --cmse-implib --out-implib=cmse_implib.o ...
 *
 * Lab 08 -- TrustZone-M: Secure/Non-Secure Partition
 * Platform: NXP FRDM-MCXN236 (Cortex-M33)
 */

#include <stdint.h>
#include "arm_cmse.h"
#include "secure_api.h"
#include "fsl_debug_console.h"

/* ── Private Secure state ────────────────────────────────────────────────── */
/*
 * s_last_adc_value lives in Secure SRAM.
 * The NS world cannot read or write this variable directly --
 * any attempt will raise a SecureFault (see Part C of the lab).
 */
static volatile int32_t s_last_adc_value = 0;

/* ── Secure_ADC_Init ─────────────────────────────────────────────────────── */
/*
 * NSC entry point -- callable from Non-Secure world.
 *
 * TODO (Part B): Verify that the PRINTF below prints with the [S] prefix,
 *       confirming the call executes in Secure state.
 */
__attribute__((cmse_nonsecure_entry))
void Secure_ADC_Init(void)
{
    PRINTF("[S] Secure_ADC_Init called from NS world\r\n");

    /* TODO: initialise real ADC peripheral here if hardware is available */
    s_last_adc_value = 0;
}

/* ── Secure_ADC_Read ─────────────────────────────────────────────────────── */
/*
 * NSC entry point -- callable from Non-Secure world.
 *
 * Returns the next simulated ADC sample.  The real version would
 * trigger an ADC conversion, wait for completion (in Secure state),
 * and return only the numeric result -- never a pointer into Secure SRAM.
 *
 * TODO (Part D): Wrap DWT_START/DWT_STOP around the call in vBenchmarkTask
 *       (NS side) and compare to a direct NS dummy function.
 */
__attribute__((cmse_nonsecure_entry))
int32_t Secure_ADC_Read(void)
{
    /* TODO: replace with real ADC read (ADC_DoSoftwareTriggerConversionSequence, etc.) */
    s_last_adc_value = (s_last_adc_value + 37) % 4096;
    return s_last_adc_value;
}
