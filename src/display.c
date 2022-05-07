#include "display.h"
#include "gpio.h"
#include "delay.h"

// CS: A4
// SCK: A5
// A0: A6
// SDA: A7
// RST: A3

#define SPI_CLK    (1 << 12)
#define SPI1_START 0x40013000
#define SPI1_CR1   (*((volatile uint32_t *)(SPI1_START + 0x00)))
#define SPI1_CR2   (*((volatile uint32_t *)(SPI1_START + 0x04)))
#define SPI1_SR    (*((volatile uint32_t *)(SPI1_START + 0x08)))
#define SPI1_DR    (*((volatile uint32_t *)(SPI1_START + 0x0C)))

#define A0 (1 << 6)
#define RST (1 << 3)

#define SET_PAGE_ADDR 0xB0
#define SET_COL_ADDR_MSB 0x10
#define SET_COL_ADDR_LSB 0x0
#define DISPLAY_ON 0xAF
#define DISPLAY_OFF 0xAE

#define NUM_COLS 128
#define NUM_PAGES 8

static void _gpio_init(void) {
    // Disable reset state
    GPIOA_CRL &= ~((1 << 14) | (1 << 18) | (1 << 22) | (1 << 26) | (1 << 30));

    // MODEy (3, 4, 5, 6, 7 2MHz out)
    GPIOA_CRL |= ((1 << 13) | (1 << 17) | (1 << 21) | (1 << 25) | (1 << 29));

    // CNFy (4, 5, 7 alt out, 3, 6 gpo)
    GPIOA_CRL |= ((1 << 19) | (1 << 23) | (1 << 31));
}

static void _spi_init(void) {
    RCC_APB2ENR |= SPI_CLK;
    for (volatile int i = 0; i < 10; i++);

    SPI1_CR1 |= (3 << 3); // CLK / 16 (fastest speed that works)
    SPI1_CR2 |= (1 << 2); // Enable SS output
    SPI1_CR1 |= (1 << 15); // 1 line mode
    SPI1_CR1 |= (1 << 14); // Transmit-only
    SPI1_CR1 |= (1 << 2); // Set as master
    SPI1_CR1 |= (1 << 6); // Enable
}

static void _display_write(uint8_t data) {
    SPI1_DR = data;
    while (!(SPI1_SR & 0x02));
    for (volatile int i = 0; i < 10; i++); // Need a very brief delay
}

void display_init(void) {
    _gpio_init();

    // Perform hardware reset of display
    GPIOA_ODR &= ~RST;
    delay(5);
    GPIOA_ODR |= RST;
    delay(1);

    _spi_init();

    // Voltage stuff
    // Not entirely sure why this is needed
    // Couldn't find much explanation in datasheet
    display_send_cmd(0x28 | 0x7);

    // Zero out display RAM and turn on
    display_clear();
    display_send_cmd(DISPLAY_ON);
}

void display_send_data(uint8_t data) {
    GPIOA_ODR |= A0;
    _display_write(data);
}

void display_send_cmd(uint8_t cmd) {
    GPIOA_ODR &= ~A0;
    _display_write(cmd);
}

void display_clear(void) {
    for (int y = 0; y < NUM_PAGES; y++) {
        display_send_cmd(SET_PAGE_ADDR | y);
        display_send_cmd(SET_COL_ADDR_MSB);
        display_send_cmd(SET_COL_ADDR_LSB);

        for (int x = 0; x < NUM_COLS; x++) {
            display_send_data(0x00);
        }
    }
}

void display_draw(uint8_t buf[64][16]) {
    for (int y = 0; y < NUM_PAGES; y++) {
        display_send_cmd(SET_PAGE_ADDR | y);
        display_send_cmd(SET_COL_ADDR_MSB);
        display_send_cmd(SET_COL_ADDR_LSB);

        for (int x = 0; x < NUM_COLS; x++) {
            int row = NUM_PAGES * y;
            uint8_t data = 0;

            for (int i = 0; i < 8; i++) {
                uint8_t bit = (buf[row + i][x / 8]) & (1 << (7 - (x % 8)));

                if (bit) {
                    data |= (1 << i);
                }
            }

            display_send_data(data);
        }
    }
}

void display_test(void) {
    for (int y = 0; y < NUM_PAGES; y++) {
        display_send_cmd(SET_PAGE_ADDR | y);
        display_send_cmd(SET_COL_ADDR_MSB);
        display_send_cmd(SET_COL_ADDR_LSB);

        for (int x = 0; x < NUM_COLS; x++) {
            display_send_data(0xFF);
            delay(50);
        }
    }
}
