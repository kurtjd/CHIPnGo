#include "spi.h"
#include "gpio.h"

#define SPI_CLK    (1 << 12)
#define SPI1_START 0x40013000
#define SPI1_CR1   (*((volatile uint32_t *)(SPI1_START + 0x00)))
#define SPI1_SR    (*((volatile uint32_t *)(SPI1_START + 0x08)))
#define SPI1_DR    (*((volatile uint32_t *)(SPI1_START + 0x0C)))

static void _gpio_init(void) {
    // MODEy (4, 5, 7 2MHz out, 6 in)
    GPIOA_CRL |= ((1 << 17) | (1 << 21) | (1 << 29));

    // CNFy (4, 5, 7 alt out, 6 floating in)
    GPIOA_CRL |= ((1 << 19) | (1 << 23) | (1 << 31));
}

void spi_init(void) {
    _gpio_init();

    RCC_APB2ENR |= SPI_CLK;
    for (volatile int i = 0; i < 10; i++);

    SPI1_CR1 |= (1 << 2); // Set as master
    SPI1_CR1 |= (1 << 6); // Enable
}

void spi_write(uint8_t data) {
    while (!(SPI1_SR & 0x02));
    SPI1_DR = data;
}
