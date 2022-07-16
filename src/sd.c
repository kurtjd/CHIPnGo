#include "sd.h"

#include <stdlib.h>

#include "delay.h"
#include "gpio.h"

// CS: A4
// SCK: A5
// MISO: A6 // PUR needed? Probably not since using breakout
// MOSI: A7 // Keep high during read transfer (after sending command)?
// Det: A9

#define SPI_CLK (1 << 12)
#define SPI1_START 0x40013000
#define SPI1_CR1 (*((volatile uint32_t *)(SPI1_START + 0x00)))
#define SPI1_CR2 (*((volatile uint32_t *)(SPI1_START + 0x04)))
#define SPI1_SR (*((volatile uint32_t *)(SPI1_START + 0x08)))
#define SPI1_DR (*((volatile uint32_t *)(SPI1_START + 0x0C)))

#define RESET_DUMMY_CYCLES 10
#define START_BITS 0x40
#define STOP_BITS 0x01
#define NUM_ARGS 4
#define NUM_R3_RESP_BYTES 5
#define READ_BYTE_DELAY 8
#define CARD_IDLE 1
#define CMD_OK 0
#define RW_OK 0xFE
#define DATA_ACCEPTED 2
#define INIT_MAX_ATTEMPTS 1000
#define READ_MAX_ATTEMPTS 1000

struct command {
    uint8_t cmd_bits;
    uint8_t args[NUM_ARGS];
    uint8_t crc;
};

const struct command GO_IDLE_STATE = {
    0,
    {0x00, 0x00, 0x00, 0x00},
    0x4A};

const struct command SEND_IF_COND = {
    8,
    {0x00, 0x00, 0x01, 0xAA},
    0x43};

const struct command APP_CMD = {
    55,
    {0x00, 0x00, 0x00, 0x00},
    0xFF};

const struct command SD_SEND_OP_COND = {
    41,
    {0x40, 0x00, 0x00, 0xA0},
    0xFF};

const struct command READ_OCR = {
    58,
    {0x00, 0x00, 0x00, 0x00},
    0xFF};

const struct command READ_SINGLE_BLOCK = {
    17,
    {0x00, 0x00, 0x00, 0x00},  // These will be replaced by addr bytes
    0xFF};

const struct command READ_MULTIPLE_BLOCK = {
    18,
    {0x00, 0x00, 0x00, 0x00},  // These will be replaced by addr bytes
    0xFF};

const struct command STOP_TRANSMISSION = {
    12,
    {0x00, 0x00, 0x00, 0x00},
    0xFF};

const struct command WRITE_BLOCK = {
    24,
    {0x00, 0x00, 0x00, 0x00},  // These will be replaced by addr bytes
    0xFF};

const struct command WRITE_MULTIPLE_BLOCK = {
    25,
    {0x00, 0x00, 0x00, 0x00},  // These will be replaced by addr bytes
    0xFF};

static void _gpio_init(void) {
    // Disable reset state
    GPIOA_CRL &= ~((1 << 18) | (1 << 22) | (1 << 30));

    // MODEy (4, 5, 7 2MHz out, 6 in)
    GPIOA_CRL |= ((1 << 17) | (1 << 21) | (1 << 29));

    // CNFy (5, 7 alt out, 6 floating in)
    GPIOA_CRL |= ((1 << 23) | (1 << 31));
}

static void _spi_init1(void) {
    RCC_APB2ENR |= SPI_CLK;
    for (volatile int i = 0; i < 10; i++)
        ;

    // CLK / 256
    // SD must be initialized with a clk speed of between 100-400KHz
    // When CPU clock is set to 72MHz, APB2 clock is set to 72Mhz
    // 72MHz / 256 equals roughly 280KHz
    SPI1_CR1 |= (7 << 3);

    SPI1_CR1 |= (1 << 9);  // Enable software CS
    SPI1_CR2 |= (1 << 2);  // Enable CS output
    SPI1_CR1 |= (1 << 2);  // Set as master
    SPI1_CR1 |= (1 << 6);  // Enable
}

static void _spi_init2(void) {
    // Wait for SPI to finish up then disable it
    delay(10);
    SPI1_CR1 &= ~(1 << 6);

    // Change CS pin to alt function output
    GPIOA_CRL |= (1 << 19);

    // Change the frequency to something much faster
    SPI1_CR1 &= ~(7 << 3);  // Erase old freq settings
    SPI1_CR1 |= (1 << 5);   // CLK / 32 (might be able to go faster)

    SPI1_CR1 &= ~(1 << 9);  // Disable software CS (thus enabling hardware CS)
    SPI1_CR1 |= (1 << 6);   // Re-enable SPI
}

static void _sd_write(uint8_t data) {
    SPI1_DR = data;
    while (!(SPI1_SR & 0x02))
        ;
}

static uint8_t _sd_read(void) {
    return SPI1_DR;
}

static void _dummy_write(int n) {
    for (int i = 0; i < n; i++)
        _sd_write(0xFF);
}

static void _rest(void) {
    // Wait until SD is sending out a constant high signal which means ready
    while (_sd_read() != 0xFF)
        _dummy_write(1);
}

static uint8_t _read_R1(void) {
    /* Must keep writing until receive a valid response,
     * or 8 bytes have been written (max time a response can take)
     * We detect a valid response by looking for a 0 in the 8th bit */
    uint8_t resp = _sd_read();
    for (int i = 0; i < READ_BYTE_DELAY && (resp & (1 << 7)); i++) {
        _dummy_write(1);
        resp = _sd_read();
    }

    return resp;
}

static const uint8_t *_read_R3(void) {
    static uint8_t resp[NUM_R3_RESP_BYTES];

    // First read the response byte. Then read in data bytes.
    resp[0] = _read_R1();
    for (int i = 1; i < NUM_R3_RESP_BYTES; i++) {
        _dummy_write(1);
        resp[i] = _sd_read();
    }

    return resp;
}

static void _power_on(void) {
    GPIOA_ODR |= (1 << 4);  // Set CS high

    // Send >74 dummy clocks with MOSI high
    _dummy_write(RESET_DUMMY_CYCLES);

    // Stabilize
    delay(10);
}

static void _send_cmd(const struct command *cmd, const uint8_t *args) {
    // Wait for SD to be ready to receive command
    _rest();

    // The full command byte must start with the start bits
    _sd_write(START_BITS | cmd->cmd_bits);

    // Send arguments
    // Use default arguments if none provided
    for (int i = 0; i < NUM_ARGS; i++) {
        if (args)
            _sd_write(args[i]);
        else
            _sd_write(cmd->args[i]);
    }

    // The full CRC byte must end with the stop bits
    _sd_write((cmd->crc << 1) | STOP_BITS);
}

static bool _reset(void) {
    // Some garbage comes in on MISO when MCU is reset without power loss
    // So do a few writes to discard it
    // TODO: Maybe come up with more elegant way to handle this
    _dummy_write(3);

    _send_cmd(&GO_IDLE_STATE, NULL);

    // Ensure SD is now in idle state
    return _read_R1() == CARD_IDLE;
}

static bool _verify(void) {
    _send_cmd(&SEND_IF_COND, NULL);

    // If the last byte is 0xAA (which means our card is SD2+), SD card is good
    return _read_R3()[NUM_R3_RESP_BYTES - 1] == 0xAA;
}

static bool _initialize(void) {
    /* The below sequence begins the SD initilization process.
     * It must be repeated until R1 returns 0
     * (signifying SD is no longer idle and ready to accept all commands) */
    int attempts = 0;
    do {
        _send_cmd(&APP_CMD, NULL);
        _read_R1();
        _send_cmd(&SD_SEND_OP_COND, NULL);
        attempts++;
    } while (attempts < INIT_MAX_ATTEMPTS && _read_R1() == CARD_IDLE);

    if (attempts >= INIT_MAX_ATTEMPTS)
        return false;

    _send_cmd(&READ_OCR, NULL);

    // Ensure SD is no longer idle and CCS is 1
    const uint8_t *resp = _read_R3();
    return ((resp[0] == CMD_OK) && (resp[1] & (1 << 6)));
}

static bool _wait_for_data_token(uint8_t token) {
    int attempts = 0;

    while (attempts < READ_MAX_ATTEMPTS && _sd_read() != token) {
        _dummy_write(1);
        attempts++;
    }

    return (attempts < READ_MAX_ATTEMPTS);
}

static bool _wait_for_data_resp(void) {
    uint8_t resp = _sd_read();
    while (resp == 0xFF) {
        _dummy_write(1);
        resp = _sd_read();
    }

    return ((resp >> 1) & 0x0F) == DATA_ACCEPTED;
}

static void _split_addr(uint32_t addr, uint8_t *buffer) {
    // Split addr into 4 argument bytes
    for (int i = 0; i < NUM_ARGS; i++)
        buffer[i] = (addr >> (24 - (i * 8))) & 0xFF;
}

static bool _read_block_data(uint8_t *buffer) {
    if (!_wait_for_data_token(RW_OK))
        return false;

    for (int i = 0; i < SD_BLOCK_SIZE; i++) {
        _dummy_write(1);
        buffer[i] = _sd_read();
    }

    // Have to read the 2 byte CRC so send a couple dummy writes
    _dummy_write(2);

    return true;
}

static bool _write_block_data(const uint8_t *buffer) {
    _sd_write(RW_OK);  // Send the packet start token

    // Send all data bytes
    for (int i = 0; i < SD_BLOCK_SIZE; i++)
        _sd_write(buffer[i]);

    _sd_write(0xFF);  // Send bogus CRC

    return _wait_for_data_resp();
}

bool sd_init(void) {
    if (!sd_inserted())
        return false;

    _gpio_init();
    _spi_init1();
    _power_on();

    // Set CS low manually since we aren't in full-blown SPI yet
    GPIOA_ODR &= ~(1 << 4);

    // Ensure all stages of sequence were successful
    if (!_reset())
        return false;
    if (!_verify())
        return false;
    if (!_initialize())
        return false;

    // Reinitialize SPI with a much faster frequency and hardware CS
    _spi_init2();
    return true;
}

bool sd_inserted(void) {
    return (GPIOA_IDR & (1 << 9));
}

bool sd_read_block(uint32_t addr, uint8_t *buffer) {
    uint8_t args[NUM_ARGS];
    _split_addr(addr, args);

    _send_cmd(&READ_SINGLE_BLOCK, args);
    if (_read_R1() == CMD_OK) {
        if (!_read_block_data(buffer))
            return false;
    }

    return true;
}

bool sd_read_blocks(uint32_t addr, uint8_t *buffer, int num_blocks) {
    uint8_t args[NUM_ARGS];
    _split_addr(addr, args);

    _send_cmd(&READ_MULTIPLE_BLOCK, args);
    if (_read_R1() == CMD_OK) {
        for (int i = 0; i < num_blocks; i++) {
            if (!_read_block_data(buffer + (i * SD_BLOCK_SIZE)))
                return false;
        }

        // Signal we wish to stop reading data
        _send_cmd(&STOP_TRANSMISSION, NULL);
        _dummy_write(1);  // Discard stuff byte
        _read_R1();
    }

    return true;
}

bool sd_write_block(uint32_t addr, const uint8_t *buffer) {
    uint8_t args[NUM_ARGS];
    _split_addr(addr, args);

    _send_cmd(&WRITE_BLOCK, args);
    if (_read_R1() == CMD_OK)
        return _write_block_data(buffer);

    return false;
}
