#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "sysclk.h"
#include "clock.h"
#include "gpio.h"
#include "uart.h"
#include "sd.h"
#include "display.h"
#include "buttons.h"
#include "pwm.h"
#include "chip8.h"

// Emulator
CHIP8 chip8;
uint16_t pc_start_addr = PC_START_ADDR_DEFAULT;
uint32_t cpu_freq = CPU_FREQ_DEFAULT;
uint32_t timer_freq = TIMER_FREQ_DEFAULT;
uint32_t refresh_freq = REFRESH_FREQ_DEFAULT;
bool play_sound = false;
bool quirks[NUM_QUIRKS] = {0, 0, 0, 0, 0, 0, 0, 0, 0};


// A basic splash screen that waits for user to press A
void show_splash(void) {
    display_print(37, 2, "CHIP N GO");
    display_print(20, 4, "PRESS A TO PLAY");
    display_print(5, 7, "CREATED BY KURT");

    while (!btn_released(BTN_A));
}


// Set up the emulator to begin running.
bool init_emulator(void)
{
    chip8_init(&chip8, cpu_freq, timer_freq, refresh_freq, pc_start_addr,
                quirks);
    chip8_load_font(&chip8);

    /* Load ROM into memory. */
    if (!chip8_load_rom(&chip8))
    {
        return false;
    }

    return true;
}

// Makes the physical screen match the emulator display.
void draw_display(void)
{
    display_draw(chip8.display);
}

// Handles sound.
void handle_sound(void)
{
    if (!play_sound && chip8.beep)
    {
        pwm_start();
        play_sound = true;
    }
    else if (play_sound && !chip8.beep)
    {
        pwm_stop();
        play_sound = false;
    }
}

// Handles drawing the display.
void handle_display(void)
{
    if (chip8.display_updated)
    {
        draw_display();
    }
}

// Checks for key presses/releases and a quit event.
void handle_input(void)
{
    // Do stuff with buttons here
    if (btn_pressed(BTN_LEFT)) {
        chip8.keypad[0x07] = KEY_DOWN;
        chip8.keypad[0x04] = KEY_DOWN;
    }
    if (btn_pressed(BTN_UP)) {
        chip8.keypad[0x05] = KEY_DOWN;
    }
    if (btn_pressed(BTN_DOWN)) {
        chip8.keypad[0x08] = KEY_DOWN;
    }
    if (btn_pressed(BTN_RIGHT)) {
        chip8.keypad[0x09] = KEY_DOWN;
        chip8.keypad[0x06] = KEY_DOWN;
    }
    if (btn_pressed(BTN_B)) {
        chip8.keypad[0x06] = KEY_DOWN;
    }

    if (btn_released(BTN_LEFT)) {
        chip8.keypad[0x07] = KEY_UP;
        chip8.keypad[0x04] = KEY_UP;
    }
    if (btn_released(BTN_UP)) {
        chip8.keypad[0x05] = KEY_UP;
    }
    if (btn_released(BTN_DOWN)) {
        chip8.keypad[0x08] = KEY_UP;
    }
    if (btn_released(BTN_RIGHT)) {
        chip8.keypad[0x09] = KEY_UP;
        chip8.keypad[0x06] = KEY_UP;
    }
    if (btn_released(BTN_B)) {
        chip8.keypad[0x06] = KEY_UP;
    }
}

void echo_sd_read(uint32_t addr) {
    uint8_t data[SD_BLOCK_SIZE * 2] = {0};
    sd_read_blocks(addr, data, 2);

    for (int i = 0; i < SD_BLOCK_SIZE * 2; i++) {
        char msg[7] = {0};
        sprintf(msg, "0x%02X ", data[i]);

        if ((i + 1) % 16 == 0)
            msg[5] = '\n';

        uart_write_str(msg);
    }

    uart_write('\n');
}

int main(void)
{
    set_sysclk(72);
    gpio_init(GPIOA);
    gpio_init(GPIOB);

    uart_init(9600);

    buttons_init();
    pwm_init(880);

    if (sd_init()) {
        uart_write_str("SD successfully initialized!\n\n");
        echo_sd_read(25088); // Just a test to ensure we actually read blocks
    } else {
        uart_write_str("SD failed to initialize.\n\n");
    }

    display_init();
    show_splash();

    init_emulator();
    clock_start();

    while (1)
    {
        handle_input();
        chip8_cycle(&chip8);
        handle_sound();
        handle_display();
    }

    return 0;
}
