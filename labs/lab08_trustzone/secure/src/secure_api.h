/*
 * secure_api.h -- Public Secure API declarations
 *
 * These declarations are included by BOTH the Secure project (for the
 * implementation) and by the Non-Secure project (via the CMSE import
 * library cmse_implib.o).
 *
 * The cmse_nonsecure_entry attribute on each function causes the linker
 * to emit an SG (Secure Gateway) veneer stub in the .gnu.sgstubs section,
 * which must be placed in an NSC region.
 *
 * Lab 08 -- TrustZone-M
 */
#ifndef SECURE_API_H
#define SECURE_API_H

#include <stdint.h>

/*
 * Secure_ADC_Init -- initialise the Secure-side ADC (simulated).
 * Called once from vSensorTask before the first Secure_ADC_Read.
 */
void Secure_ADC_Init(void);

/*
 * Secure_ADC_Read -- return the next simulated ADC sample [0..4095].
 * The internal ADC state (s_last_adc_value) is never accessible from NS.
 */
int32_t Secure_ADC_Read(void);

#endif /* SECURE_API_H */
