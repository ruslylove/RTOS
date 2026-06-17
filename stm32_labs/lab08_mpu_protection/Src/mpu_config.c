#include "stm32h5xx_hal.h"
#include "mpu_config.h"
#include <stdio.h>

/*
 * 32-byte aligned so it occupies exactly one ARM v8-M MPU granule.
 * The linker places this in .data; the MPU region makes it read-only
 * at runtime so any write attempt triggers a MemManage fault.
 */
__attribute__((aligned(32))) uint8_t secure_buffer[32] = "SECRET: read-only protected data";

void mpu_init(void)
{
    /* Disable MPU before reprogramming */
    MPU->CTRL = 0U;
    __DSB();
    __ISB();

    /*
     * MAIR0 byte 0 (AttrIndex = 0): Normal memory, Non-Cacheable
     *   Upper nibble (Outer) = 0x4: non-cacheable
     *   Lower nibble (Inner) = 0x4: non-cacheable
     */
    MPU->MAIR0 = 0x44U;

    /*
     * Region 0 — secure_buffer: Read-Only, Execute-Never
     *
     * RBAR layout (ARMv8-M):
     *   [31:5] BASE  = base address of region (32-byte aligned)
     *   [4:3]  SH    = 0b00 non-shareable (single-core STM32H503)
     *   [2:1]  AP    = 0b10 Read-Only privileged (writes fault even from priv code)
     *   [0]    XN    = 1    Execute-Never (data region, never run code here)
     *
     * RLAR layout:
     *   [31:5] LIMIT = last 32-byte granule of region (inclusive upper boundary)
     *   [3:1]  AttrIndx = 0 (uses MAIR0 byte 0 = normal non-cacheable)
     *   [0]    EN    = 1  region enabled
     */
    MPU->RNR  = 0U;
    MPU->RBAR = ((uint32_t)secure_buffer & 0xFFFFFFE0U)
                | (0U << 3)
                | (MPU_AP_RO_PRIV << 1)
                | (1U);
    MPU->RLAR = (((uint32_t)secure_buffer + (uint32_t)sizeof(secure_buffer) - 1U) & 0xFFFFFFE0U)
                | (0U << 1)
                | (1U);

    /* Route MemManage violations to MemManage_Handler, not HardFault */
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;

    /*
     * Enable MPU with PRIVDEFENA=1:
     *   - Region 0 overrides the default map for secure_buffer addresses
     *   - Everything else (flash, RAM, peripherals) uses the ARM default
     *     memory map and remains accessible to privileged code
     */
    __DMB();
    MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;
    __DSB();
    __ISB();
}

void mpu_dump_config(void)
{
    printf("── MPU Configuration ──\r\n");
    printf("  TYPE : 0x%08lX  (%lu regions supported)\r\n",
           (unsigned long)MPU->TYPE,
           (unsigned long)((MPU->TYPE >> 8) & 0xFFU));
    printf("  CTRL : 0x%08lX  ENABLE=%lu  HFNMIENA=%lu  PRIVDEFENA=%lu\r\n",
           (unsigned long)MPU->CTRL,
           (unsigned long)(MPU->CTRL        & 1U),
           (unsigned long)((MPU->CTRL >> 1) & 1U),
           (unsigned long)((MPU->CTRL >> 2) & 1U));

    MPU->RNR = 0U;
    printf("  Region 0: RBAR=0x%08lX  RLAR=0x%08lX\r\n",
           (unsigned long)MPU->RBAR, (unsigned long)MPU->RLAR);
    printf("    Base=0x%08lX  AP=0b%lu%lu  XN=%lu  EN=%lu\r\n",
           (unsigned long)(MPU->RBAR & 0xFFFFFFE0U),
           (unsigned long)((MPU->RBAR >> 2) & 1U),
           (unsigned long)((MPU->RBAR >> 1) & 1U),
           (unsigned long)(MPU->RBAR & 1U),
           (unsigned long)(MPU->RLAR & 1U));
    printf("    secure_buffer @ 0x%08lX  size=%u bytes\r\n",
           (unsigned long)secure_buffer, (unsigned)sizeof(secure_buffer));
    printf("────────────────────────\r\n\r\n");
}
