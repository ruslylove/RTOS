/*
 * Bare-metal LPUART4 driver for FRDM-MCXN236.
 *
 * Hardware facts (from MCXN236 Reference Manual):
 *   LP_FLEXCOMM4 = LPUART4, base address 0x400B4000
 *   Clock source: FRO 96 MHz → FCCLKSEL[4] = 3 (fro_hf_div = 96 MHz)
 *   Pins: PORT1 pin 8 (TX), PORT1 pin 9 (RX), mux = ALT2
 *
 * Baud rate calculation (96 MHz, 115200):
 *   OSR = 15 (16× oversampling, default)
 *   SBR = 96000000 / (16 × 115200) = 52.08 ≈ 52
 *   Actual baud = 96000000 / (16 × 52) = 115384 (0.16% error)
 */

#include "uart.h"
#include <stdarg.h>
#include <string.h>

/* ── Peripheral base addresses ─────────────────────────────────────── */
#define SYSCON0_BASE    0x40000000u
#define PORT1_BASE      0x40117000u
#define LP_FC4_BASE     0x400B4000u   /* LP_FLEXCOMM4 / LPUART4 */

/* ── SYSCON0 register offsets ──────────────────────────────────────── */
#define SYSCON_PRESETCTRL1  (*(volatile uint32_t *)(SYSCON0_BASE + 0x104u))
#define SYSCON_AHBCLKCTRL0  (*(volatile uint32_t *)(SYSCON0_BASE + 0x200u))
#define SYSCON_AHBCLKCTRL1  (*(volatile uint32_t *)(SYSCON0_BASE + 0x204u))
/* FCCLKSEL[n]: array at 0x2B0, step 4; index 4 → offset 0x2C0 */
#define SYSCON_FCCLKSEL4    (*(volatile uint32_t *)(SYSCON0_BASE + 0x2C0u))

/* ── PORT1 pin control registers (PCR[n] at offset n×4) ───────────── */
#define PORT1_PCR8  (*(volatile uint32_t *)(PORT1_BASE + 0x20u))  /* pin 8 */
#define PORT1_PCR9  (*(volatile uint32_t *)(PORT1_BASE + 0x24u))  /* pin 9 */
#define PORT_PCR_MUX_SHIFT  8u
#define PORT_PCR_MUX_ALT2   (2u << PORT_PCR_MUX_SHIFT)

/* ── LP_FLEXCOMM4 registers ────────────────────────────────────────── */
#define LP_FC4_PSELID   (*(volatile uint32_t *)(LP_FC4_BASE + 0xFF8u))
#define LP_FC4_PSELID_UART  1u   /* PERSEL = 1 → UART */

/* ── LPUART4 registers (share LP_FLEXCOMM4 base) ──────────────────── */
#define LPUART4_BAUD    (*(volatile uint32_t *)(LP_FC4_BASE + 0x10u))
#define LPUART4_STAT    (*(volatile uint32_t *)(LP_FC4_BASE + 0x14u))
#define LPUART4_CTRL    (*(volatile uint32_t *)(LP_FC4_BASE + 0x18u))
#define LPUART4_DATA    (*(volatile uint32_t *)(LP_FC4_BASE + 0x1Cu))

#define LPUART_BAUD_OSR_SHIFT   24u
#define LPUART_BAUD_SBR_MASK    0x1FFFu
#define LPUART_CTRL_TE          (1u << 19)  /* Transmitter Enable */
#define LPUART_CTRL_RE          (1u << 18)  /* Receiver Enable */
#define LPUART_STAT_TDRE        (1u << 23)  /* TX Data Register Empty */

/* ── Bit helpers ───────────────────────────────────────────────────── */
#define BIT(n)  (1u << (n))

void uart_init(void)
{
    /* 1. Enable AHB clock to PORT1 (AHBCLKCTRL0 bit 14) */
    SYSCON_AHBCLKCTRL0 |= BIT(14);

    /* 2. Enable AHB clock to LP_FLEXCOMM4 (AHBCLKCTRL1 bit 15) */
    SYSCON_AHBCLKCTRL1 |= BIT(15);

    /* 3. Release LP_FLEXCOMM4 from reset (PRESETCTRL1 bit 15, write 0 to release) */
    SYSCON_PRESETCTRL1 &= ~BIT(15);

    /* 4. Select FRO 96 MHz (fro_hf_div) as LP_FLEXCOMM4 clock: FCCLKSEL[4] = 3 */
    SYSCON_FCCLKSEL4 = 3u;

    /* 5. Mux PORT1 pin 8 → LP_FLEXCOMM4_TXD (ALT2) */
    PORT1_PCR8 = PORT_PCR_MUX_ALT2;
    /* 6. Mux PORT1 pin 9 → LP_FLEXCOMM4_RXD (ALT2) */
    PORT1_PCR9 = PORT_PCR_MUX_ALT2;

    /* 7. Select UART mode in LP_FLEXCOMM4 */
    LP_FC4_PSELID = LP_FC4_PSELID_UART;

    /* 8. Configure LPUART4 baud rate: 96 MHz, OSR=15 (16×), SBR=52 → ~115384 baud */
    LPUART4_BAUD = ((15u << LPUART_BAUD_OSR_SHIFT) | (52u & LPUART_BAUD_SBR_MASK));

    /* 9. Enable TX and RX */
    LPUART4_CTRL |= LPUART_CTRL_TE | LPUART_CTRL_RE;
}

void uart_putc(char c)
{
    while (!(LPUART4_STAT & LPUART_STAT_TDRE))
        ;
    LPUART4_DATA = (uint32_t)(uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

/* Minimal printf — supports %d, %u, %s, %c, %x */
void uart_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            if (*p == '\n') uart_putc('\r');
            uart_putc(*p);
            continue;
        }
        p++;
        switch (*p) {
        case 'd': {
            int v = va_arg(ap, int);
            char buf[12];
            int i = 0;
            if (v < 0) { uart_putc('-'); v = -v; }
            if (v == 0) { uart_putc('0'); break; }
            while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; }
            while (i--) uart_putc(buf[i]);
            break;
        }
        case 'u': {
            unsigned v = va_arg(ap, unsigned);
            char buf[12]; int i = 0;
            if (v == 0) { uart_putc('0'); break; }
            while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; }
            while (i--) uart_putc(buf[i]);
            break;
        }
        case 'x': {
            unsigned v = va_arg(ap, unsigned);
            char buf[8]; int i = 0;
            if (v == 0) { uart_puts("0"); break; }
            while (v > 0) {
                int d = v & 0xF;
                buf[i++] = d < 10 ? '0' + d : 'a' + d - 10;
                v >>= 4;
            }
            while (i--) uart_putc(buf[i]);
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            uart_puts(s);
            break;
        }
        case 'c':
            uart_putc((char)va_arg(ap, int));
            break;
        case '%':
            uart_putc('%');
            break;
        default:
            uart_putc('%');
            uart_putc(*p);
            break;
        }
    }
    va_end(ap);
}
