#include "sd.h"
#include "gpio.h"

#define SPI_CLK    (1 << 14)
#define SPI2_START 0x40013800
#define SPI2_CR1   (*((volatile uint32_t *)(SPI2_START + 0x00)))
#define SPI2_SR    (*((volatile uint32_t *)(SPI2_START + 0x08)))
#define SPI2_DR    (*((volatile uint32_t *)(SPI2_START + 0x0C)))

static void _gpio_init(void) {
    // MODEy (12, 13, 15 2MHz out, 14 in)
    GPIOB_CRH |= ((1 << 17) | (1 << 21) | (1 << 29));

    // CNFy (12, 13, 15 alt out, 14 floating in)
    GPIOB_CRH |= ((1 << 19) | (1 << 23) | (1 << 31));
}

void sd_init(void) {
    _gpio_init();

    RCC_APB1ENR |= SPI_CLK;
    for (volatile int i = 0; i < 10; i++);

    SPI2_CR1 |= (1 << 2); // Set as master
    SPI2_CR1 |= (1 << 6); // Enable
}

void sd_write(uint8_t data) {
    while (!(SPI2_SR & 0x02));
    SPI2_DR = data;
}

uint8_t sd_read(void) {
    return SPI2_DR;
}

bool sd_inserted(void) {
    return (GPIOA_IDR & (1 << 8));
}
