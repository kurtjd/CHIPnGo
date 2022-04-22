#include <stdbool.h>
#include <stdint.h>
#include "sysclk.h"
#include "clock.h"
#include "gpio.h"
#include "uart.h"
#include "spi.h"
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
    for (int y = 0; y < DISPLAY_HEIGHT; y++)
    {
        for (int x = 0; x < DISPLAY_WIDTH_BYTES; x++)
        {
            uart_write(chip8.display[y][x]);
        }
    }
}

// Handles sound.
void handle_sound(void)
{
    if (!play_sound && chip8.beep)
    {
        play_sound = true;
    }
    else if (play_sound && !chip8.beep)
    {
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
bool handle_input(void)
{
    // Do stuff with buttons here
    if (btn_pressed(BTN_LEFT)) {
        chip8.keypad[0x07] = KEY_DOWN;
    }
    if (btn_pressed(BTN_UP)) {
        chip8.keypad[0x05] = KEY_DOWN;
    }
    if (btn_pressed(BTN_DOWN)) {
        chip8.keypad[0x08] = KEY_DOWN;
    }
    if (btn_pressed(BTN_RIGHT)) {
        chip8.keypad[0x09] = KEY_DOWN;
    }
    if (btn_pressed(BTN_B)) {
        chip8.keypad[0x06] = KEY_DOWN;
    }

    if (btn_released(BTN_LEFT)) {
        chip8.keypad[0x07] = KEY_UP;
    }
    if (btn_released(BTN_UP)) {
        chip8.keypad[0x05] = KEY_UP;
    }
    if (btn_released(BTN_DOWN)) {
        chip8.keypad[0x08] = KEY_UP;
    }
    if (btn_released(BTN_RIGHT)) {
        chip8.keypad[0x09] = KEY_UP;
    }
    if (btn_released(BTN_B)) {
        chip8.keypad[0x06] = KEY_UP;
    }

    return true;
}

int main(void)
{
    set_sysclk(72);
    gpio_init(GPIOA);
    gpio_init(GPIOB);
    buttons_init();
    spi_init();
	uart_init(500000);
    pwm_init(440);

    // Wait for button press here
    while (!btn_released(BTN_B));
	
    init_emulator();
    clock_start();

    while (!chip8.exit && handle_input())
    {
        chip8_cycle(&chip8);
        handle_sound();
        handle_display();
    }

    return 0;
}
