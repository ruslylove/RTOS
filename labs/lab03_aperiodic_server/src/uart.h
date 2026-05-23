#ifndef UART_H
#define UART_H

#include <stdint.h>

/* Bare-metal LPUART4 driver — implementation in uart.c */
void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_printf(const char *fmt, ...);

#endif /* UART_H */
