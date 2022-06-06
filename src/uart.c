#include "uart.h"

#include "gpio.h"
#include "sysclk.h"

#define NVIC 0xE000E100
#define NVIC_ISER1 (*((volatile uint32_t *)(NVIC + 0x04)))

#define UART1 0x40013800
#define UART1_CR1 (*((volatile uint32_t *)(UART1 + 0x0C)))
#define UART1_CR2 (*((volatile uint32_t *)(UART1 + 0x10)))
#define UART1_CR3 (*((volatile uint32_t *)(UART1 + 0x14)))
#define UART1_SR (*((volatile uint32_t *)(UART1 + 0x00)))
#define UART1_DR (*((volatile uint32_t *)(UART1 + 0x04)))
#define UART1_BRR (*((volatile uint32_t *)(UART1 + 0x08)))

// Enable PORTA for UART1
static void _gpio_init(void) {
    GPIOA_CRH |= 0xA0;   // Configure Tx
    GPIOA_CRH |= 0x400;  // Configure Rx
}

// Enable and initialize UART settings
void uart_init(int baud_rate) {
    _gpio_init();

    RCC_APB2ENR |= (1 << 14);  // Set clock
    for (volatile int i = 0; i < 10; i++)
        ;

    // Calculate baud rate divisor
    float brd = APB2_CLOCK_SPEED / (baud_rate * 16);
    int mantissa = brd;
    int fraction = (brd - mantissa) * 16;
    UART1_BRR = (mantissa << 4) | fraction;

    UART1_CR1 |= 0x200C;  // Enable UART tx and rx
}

// Sends a byte over UART
void uart_write(uint8_t data) {
    while (!uart_tx_empty())
        ;  // Wait for tx buffer to be empty
    UART1_DR = data;
}

void uart_write_str(const char *str) {
    while (*str) {
        uart_write(*str++);
    }
}

uint8_t uart_read(void) {
    return UART1_DR;
}

void uart_en_rx_int(void) {
    UART1_CR1 |= 0x20;
    NVIC_ISER1 |= 0x20;
}

bool uart_rx_empty(void) {
    return !(UART1_SR & 0x20);
}

bool uart_tx_empty(void) {
    return UART1_SR & 0x80;
}
