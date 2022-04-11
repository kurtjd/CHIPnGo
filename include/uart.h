#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

void uart_init(int baud_rate);
void uart_write(uint8_t data);
void uart_write_str(const char *str);
uint8_t uart_read(void);
void uart_en_rx_int(void);
bool uart_rx_empty(void);
bool uart_tx_empty(void);

#endif
