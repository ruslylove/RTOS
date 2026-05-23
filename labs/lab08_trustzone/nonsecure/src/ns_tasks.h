/*
 * ns_tasks.h -- Non-Secure task declarations
 *
 * Lab 08 -- TrustZone-M: Secure/Non-Secure Partition
 */
#ifndef NS_TASKS_H
#define NS_TASKS_H

#include "FreeRTOS.h"
#include "task.h"

/*
 * vSensorTask -- calls Secure_ADC_Init once, then reads Secure_ADC_Read
 *                every 200 ms via the NSC veneer.
 *                Part B: Normal operation.
 *                Part C: Uncomment the deliberate fault to trigger SecureFault.
 */
void vSensorTask(void *pv);

/*
 * vBenchmarkTask -- measures the cycle overhead of an NSC call vs a direct
 *                   NS function call using the DWT cycle counter.
 *                   Runs once and calls vTaskDelete(NULL).
 */
void vBenchmarkTask(void *pv);

/*
 * ns_dummy_adc_read -- Non-Secure dummy ADC with the same logic as
 *                      Secure_ADC_Read, used as the baseline in the
 *                      benchmark.
 */
int32_t ns_dummy_adc_read(void);

#endif /* NS_TASKS_H */
