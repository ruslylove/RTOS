/*
 * main_secure.c -- Secure-world entry point
 *
 * Responsibilities:
 *   1. Configure the Security Attribution Unit (SAU).
 *   2. Run Secure diagnostics (TZM_IsSecure probe).
 *   3. Release the Non-Secure image.
 *
 * Lab 08 -- TrustZone-M: Secure/Non-Secure Partition
 * Platform: NXP FRDM-MCXN236 (Cortex-M33)
 */

#include <stdint.h>
#include <stdbool.h>
#include "arm_cmse.h"
#include "MCXN236.h"        /* CMSIS device header -- SAU, SCB, etc. */
#include "uart.h"
#include "secure_api.h"

/* ── Forward declarations ────────────────────────────────────────────────── */
static void SAU_Configure(void);
static bool TZM_IsSecure(uint32_t addr);
static void Boot_NS(void);

/* ── Entry point ─────────────────────────────────────────────────────────── */
int main(void)
{
    /* TODO: call BOARD_InitHardware() or equivalent SDK init */

    uart_printf("[S] Secure world started\r\n");

    /* --- Part A: configure the SAU ---------------------------------------- */
    SAU_Configure();
    uart_printf("[S] SAU configured\r\n");

    /* --- Part A: probe three addresses ------------------------------------- */
    uart_printf("[S] 0x00010000 is %s\r\n",
                TZM_IsSecure(0x00010000U) ? "SECURE" : "NS/NSC");
    uart_printf("[S] 0x00050000 is %s\r\n",
                TZM_IsSecure(0x00050000U) ? "SECURE" : "NS/NSC");
    uart_printf("[S] 0x0003FE00 is %s (NSC)\r\n",
                TZM_IsSecure(0x0003FE00U) ? "SECURE" : "NS/NSC");

    /* --- Part B: release the Non-Secure core ------------------------------ */
    uart_printf("[S] Releasing Non-Secure image at 0x00040000\r\n");
    Boot_NS();

    /* Should not return -- Non-Secure world takes over execution */
    for (;;) {}
}

/* ── SAU_Configure ───────────────────────────────────────────────────────── */
/*
 * Configure the Security Attribution Unit.
 *
 * Region map:
 *   0: NS  flash  0x0004_0000 -- 0x001F_FFFF
 *   1: NSC flash  0x0003_FE00 -- 0x0003_FFFF  (512 B, veneer stubs here)
 *   2: NS  SRAM   0x2002_0000 -- 0x200F_FFFF
 *
 * All other addresses default to Secure (SAU disabled = all Secure when
 * SAU_CTRL.ALLNS = 0, which is the reset default).
 */
static void SAU_Configure(void)
{
    /* TODO: Disable SAU before programming regions */
    SAU->CTRL = 0;

    /* TODO: Region 0 -- Non-Secure flash */
    SAU->RNR  = 0;
    SAU->RBAR = 0x00040000U;
    SAU->RLAR = (0x001FFFFFU & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

    /* TODO: Region 1 -- NSC (Non-Secure Callable) region for SG stubs */
    SAU->RNR  = 1;
    SAU->RBAR = 0x0003FE00U;
    SAU->RLAR = (0x0003FFFFU & SAU_RLAR_LADDR_Msk)
              | SAU_RLAR_NSC_Msk | SAU_RLAR_ENABLE_Msk;

    /* TODO: Region 2 -- Non-Secure SRAM */
    SAU->RNR  = 2;
    SAU->RBAR = 0x20020000U;
    SAU->RLAR = (0x200FFFFFU & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;

    /* TODO: Enable SAU and flush pipeline */
    SAU->CTRL = SAU_CTRL_ENABLE_Msk;
    __DSB();
    __ISB();
}

/* ── TZM_IsSecure ────────────────────────────────────────────────────────── */
/*
 * Return true if addr is in a Secure region (i.e., NOT in any NS or NSC
 * SAU region).  This function is Secure-side only -- NS code must not and
 * cannot call it.
 */
static bool TZM_IsSecure(uint32_t addr)
{
    /* TODO: iterate SAU regions 0..7; return false if addr falls in NS/NSC */
    for (int i = 0; i < 8; i++) {
        SAU->RNR = (uint32_t)i;
        if (!(SAU->RLAR & SAU_RLAR_ENABLE_Msk)) continue;
        uint32_t base  = SAU->RBAR & SAU_RBAR_BADDR_Msk;
        uint32_t limit = (SAU->RLAR & SAU_RLAR_LADDR_Msk) | 0x1FU;
        if (addr >= base && addr <= limit) {
            return false; /* addr is NS or NSC */
        }
    }
    return true; /* addr is Secure */
}

/* ── Boot_NS ─────────────────────────────────────────────────────────────── */
/*
 * Transfer control to the Non-Secure image.
 *
 * Steps:
 *   1. Read the Non-Secure vector table from the NS flash start.
 *   2. Set the NS MSP from the first word (initial stack pointer).
 *   3. Call the NS reset handler via BLXNS.
 *
 * The NS image is linked to start at 0x0004_0000.
 */
static void Boot_NS(void)
{
    /* TODO: implement NS release using CMSE BLXNS or __TZ_set_MSP_NS() */

    /* Non-Secure vector table base */
    const uint32_t NS_VTOR = 0x00040000U;

    /* Set Non-Secure vector table offset register */
    SCB_NS->VTOR = NS_VTOR;
    __DSB();

    /* Read NS initial SP and reset handler from the NS vector table */
    uint32_t ns_sp    = *(volatile uint32_t *)(NS_VTOR);
    uint32_t ns_reset = *(volatile uint32_t *)(NS_VTOR + 4U);

    /* TODO: set NS MSP using __TZ_set_MSP_NS(ns_sp) */
    __TZ_set_MSP_NS(ns_sp);

    /* TODO: call NS reset handler via function pointer with BLXNS */
    /* Cast to non-secure function pointer (bit 0 clear = NS call) */
    typedef void (*ns_func_t)(void) __attribute__((cmse_nonsecure_call));
    ns_func_t ns_entry = (ns_func_t)(ns_reset & ~1U);
    ns_entry();
}
