#include <stdint.h>
#include <math.h>
#include <string.h>
#include "dwt.h"
#include "workload.h"

/* ── Integer square-root (Newton-Raphson, 8 iterations) ──────────────────── */
uint32_t workload_sqrt(float x)
{
    dwt_reset();
    volatile float r = sqrtf(x);          /* uses Cortex-M33 FPU sqrt */
    (void)r;
    return dwt_cycles();
}

/* ── 256-byte memcpy ──────────────────────────────────────────────────────── */
static uint8_t s_src[256];
static uint8_t s_dst[256];

uint32_t workload_memcpy_256(void)
{
    /* Initialise source */
    for (int i = 0; i < 256; i++) s_src[i] = (uint8_t)i;
    dwt_reset();
    memcpy(s_dst, s_src, 256);
    return dwt_cycles();
}

/* ── 16-tap FIR on 64 samples ────────────────────────────────────────────── */
static const float fir_coeff[16] = {
    0.0f, 0.05f, 0.1f, 0.15f, 0.2f, 0.15f, 0.1f, 0.05f,
    0.05f, 0.1f, 0.15f, 0.2f, 0.15f, 0.1f, 0.05f, 0.0f
};
static float fir_input[64 + 16];
static float fir_output[64];

uint32_t workload_fir_filter(void)
{
    for (int i = 0; i < 80; i++) fir_input[i] = (float)(i % 32);
    dwt_reset();
    for (int n = 0; n < 64; n++) {
        float acc = 0.0f;
        for (int k = 0; k < 16; k++)
            acc += fir_coeff[k] * fir_input[n + k];
        fir_output[n] = acc;
    }
    return dwt_cycles();
}

/* ── Bubble-sort of 128 uint16_t values ─────────────────────────────────── */
static uint16_t sort_buf[128];

uint32_t workload_sort_128(void)
{
    for (int i = 0; i < 128; i++) sort_buf[i] = (uint16_t)(127 - i);
    dwt_reset();
    for (int i = 0; i < 127; i++) {
        for (int j = 0; j < 127 - i; j++) {
            if (sort_buf[j] > sort_buf[j + 1]) {
                uint16_t tmp = sort_buf[j];
                sort_buf[j]  = sort_buf[j + 1];
                sort_buf[j + 1] = tmp;
            }
        }
    }
    return dwt_cycles();
}

/* ── Dot-product of two 32-element float arrays ─────────────────────────── */
static float va[32], vb[32];

uint32_t workload_fpu_dot(void)
{
    for (int i = 0; i < 32; i++) { va[i] = (float)i; vb[i] = (float)(31 - i); }
    dwt_reset();
    volatile float dot = 0.0f;
    for (int i = 0; i < 32; i++) dot += va[i] * vb[i];
    (void)dot;
    return dwt_cycles();
}
