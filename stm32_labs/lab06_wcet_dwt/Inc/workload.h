#ifndef WORKLOAD_H
#define WORKLOAD_H

#include <stdint.h>

/* Reference workloads for WCET measurement */
uint32_t workload_sqrt(float x);         /* integer square-root via Newton */
uint32_t workload_memcpy_256(void);      /* copy 256 bytes, return cycles */
uint32_t workload_fir_filter(void);      /* 16-tap FIR on 64 samples, return cycles */
uint32_t workload_sort_128(void);        /* bubble-sort 128 uint16_t, return cycles */

/* FPU workload — uses Cortex-M33 FPU */
uint32_t workload_fpu_dot(void);         /* dot-product of two 32-element float arrays */

#endif
