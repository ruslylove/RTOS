/*
 * uart.h -- Shared UART helper declaration
 *
 * Used by both Secure and Non-Secure projects.
 * Implemented in uart.c in each sub-project.
 */
#ifndef UART_H
#define UART_H

#include <stdint.h>

/*
 * uart_printf - lightweight printf over LPUART4.
 * Supports %s, %d, %u, %lu, %ld, %X, %08lX format specifiers.
 * Not re-entrant; protect with a critical section if called from
 * multiple tasks.
 */
void uart_printf(const char *fmt, ...);

#endif /* UART_H */
