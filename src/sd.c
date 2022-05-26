#include "sd.h"
#include "gpio.h"
#include "delay.h"

// CS: B12
// SCK: B13
// MISO: B14 // PUR needed? Probably not since using breakout
// MOSI: B15 // Keep high during read transfer (after sending command)?

#define SPI_CLK    (1 << 14)
#define SPI2_START 0x40003800
#define SPI2_CR1   (*((volatile uint32_t *)(SPI2_START + 0x00)))
#define SPI2_CR2   (*((volatile uint32_t *)(SPI2_START + 0x04)))
#define SPI2_SR    (*((volatile uint32_t *)(SPI2_START + 0x08)))
#define SPI2_DR    (*((volatile uint32_t *)(SPI2_START + 0x0C)))

#define DUMMY_CYCLES 100
#define START_BITS 0x40
#define NUM_ARGS 4
#define NUM_R3_RESP_BYTES 5
#define READ_BYTE_DELAY 8

struct command {
    uint8_t cmd_bits;
    uint8_t args[NUM_ARGS];
    uint8_t crc;
};

const struct command GO_IDLE_STATE = {
    0,
    {0x00, 0x00, 0x00, 0x00},
    0x4A
};

const struct command SEND_IF_COND = {
    8,
    {0x00, 0x00, 0x01, 0xAA},
    0x43
};

const struct command APP_CMD = {
    55,
    {0x00, 0x00, 0x00, 0x00},
    0xFF
};

const struct command SD_SEND_OP_COND = {
    41,
    {0x40, 0x00, 0x00, 0xA0},
    0xFF
};

const struct command READ_OCR = {
    58,
    {0x00, 0x00, 0x00, 0x00},
    0xFF
};

static void _gpio_init1(void) {
    // Disable reset state
    GPIOB_CRH &= ~((1 << 18) | (1 << 22) | (1 << 30));

    // MODEy (12, 13, 15 2MHz out, 14 in)
    GPIOB_CRH |= ((1 << 17) | (1 << 21) | (1 << 29));

    // CNFy (13, 15 alt out, 14 floating in)
    GPIOB_CRH |= ((1 << 23) | (1 << 31));
}

static void _spi_init1(void) {
    RCC_APB1ENR |= SPI_CLK;
    for (volatile int i = 0; i < 10; i++);

    // CLK / 128
    // SD must be initialized with a clk speed of between 100-400KHz
    // When CPU clock is set to 72MHz, APB1 clock is set to 36Mhz
    // 36MHz / 128 equals roughly 280KHz
    SPI2_CR1 |= (3 << 4);

    SPI2_CR1 |= (1 << 9); // Enable software CS
    SPI2_CR2 |= (1 << 2); // Enable CS output
    SPI2_CR1 |= (1 << 2); // Set as master
    SPI2_CR1 |= (1 << 6); // Enable
}

static void _spi_init2(void) {
    // Wait for SPI to finish up then disable it
    delay(10);
    SPI2_CR1 &= ~(1 << 6);

    // Change CS pin to alt function output
    GPIOB_CRH |= (1 << 19);

    // Change the frequency to something much faster
    SPI2_CR1 &= ~(3 << 4); // Erase old freq settings
    SPI2_CR1 |= (1 << 5); // CLK / 32 (might be able to go faster)

    SPI2_CR1 &= ~(1 << 9); // Disable software CS (thus enabling hardware CS)
    SPI2_CR1 |= (1 << 6); // Enable SPI
}

static void _rest(void) {
    // Wait until SD is sending out a constant high signal which means ready
    while (sd_read() != 0xFF)
        sd_write(0xFF);
}

static uint8_t _read_R1(void) {
    /* Must keep writing 1 until receive a valid response,
     * or 8 bytes have been written (max time a response can take)
     * We detect a valid response by looking for a 0 in the 8th bit */
    uint8_t resp = sd_read();
    for (int i = 0; i < READ_BYTE_DELAY && (resp & (1 << 7)); i++) {
        sd_write(0xFF);
        resp = sd_read();
    }

    return resp;
}

static const uint8_t* _read_R3(void) {
    static uint8_t resp[NUM_R3_RESP_BYTES];

    // First read the response byte. The read the next 4 data bytes.
    resp[0] = _read_R1();
    for (int i = 1; i < NUM_R3_RESP_BYTES; i++) {
        sd_write(0xFF);
        resp[i] = sd_read();
    }

    return resp;
}

static void _power_on(void) {
    GPIOB_ODR |= (1 << 12); // Set CS high

    // Send >74 dummy clocks with MOSI high (which is why just write 0xFF)
    for (int i = 0; i < DUMMY_CYCLES; i++)
        sd_write(0xFF);

    // Stabilize
    delay(10);
}

static void _send_cmd(const struct command *cmd) {
    // Wait for SD to be ready to receive command
    _rest();

    // The full command byte must start with the start bits
    sd_write(cmd->cmd_bits | START_BITS);

    // Send arguments
    for (int i = 0; i < NUM_ARGS; i++)
        sd_write(cmd->args[i]);

    // The full CRC byte must end with a stop bit of 1
    sd_write((cmd->crc << 1) | 1);
}

static bool _reset(void) {
    // Some garbage comes in on MISO when MCU is reset without power loss
    // So do a few writes to discard it
    for (int i = 0; i < 3; i++)
        sd_write(0xFF);

    _send_cmd(&GO_IDLE_STATE);

    // A response of 0x01 indicates SD is now idle
    uint8_t resp = _read_R1();
    return resp == 1;
}

static bool _verify(void) {
    _send_cmd(&SEND_IF_COND);

    // If the last byte is 0xAA (which means our card is SD2+), SD card is good
    return _read_R3()[NUM_R3_RESP_BYTES - 1] == 0xAA;
}

static bool _initialize(void) {
    /* The below sequence begins the SD initilization process.
     * It must be repeated until R1 returns 0 (signifying SD is no longer idle) */
    do {
        _send_cmd(&APP_CMD);
        _read_R1();
        _send_cmd(&SD_SEND_OP_COND);
    } while (_read_R1() == 1); // Should implement timeout just in case

    _send_cmd(&READ_OCR);

    // Ensure SD is no longer idle and CCS is 1
    const uint8_t *resp = _read_R3();
    return ((resp[0] == 0) && (resp[1] & (1 << 6)));
}

bool sd_init(void) {
    _gpio_init1();
    _spi_init1();
    _power_on();

    // Set CS low manually since we aren't in full-blown SPI yet
    GPIOB_ODR &= ~(1 << 12);

    // Ensure all stages of sequence were successful
    if (!_reset()) return false;
    if (!_verify()) return false;
    if (!_initialize()) return false;

    // Reinitialize SPI with a much faster frequency and hardware CS
    _spi_init2();
    sd_write(0xFF);

    return true;
}

void sd_write(uint8_t data) {
    SPI2_DR = data;
    while (!(SPI2_SR & 0x02));
}

uint8_t sd_read(void) {
    return SPI2_DR;
}

bool sd_inserted(void) {
    return (GPIOA_IDR & (1 << 8));
}
