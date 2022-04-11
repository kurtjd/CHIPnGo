#include <stdbool.h>
#include <stdint.h>
#include "sysclk.h"
#include "clock.h"
#include "uart.h"
#include "buttons.h"
#include "chip8.h"

#define BAD_KEY 0x42

// Emulator
CHIP8 chip8;
uint16_t pc_start_addr = PC_START_ADDR_DEFAULT;
uint32_t cpu_freq = CPU_FREQ_DEFAULT;
uint32_t timer_freq = TIMER_FREQ_DEFAULT;
uint32_t refresh_freq = REFRESH_FREQ_DEFAULT;
bool play_sound = false;
bool quirks[NUM_QUIRKS] = {0, 0, 0, 0, 0, 0, 0, 0, 0};


/*static volatile char keypressed = '0';
static bool keyup = false;*/

/*void USART1_IRQHandler(void) {
	while (uart_rx_empty());
	if (keypressed == '0') {
		keypressed = uart_read();
	} else {
		keyup = uart_read();
	}
}*/


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

// Converts a key to the respective key on the emulator keypad.
/*uint8_t key_to_hex(char key)
{
    switch (key)
    {
    case '1':
        return 0x01;
    case '2':
        return 0x02;
    case '3':
        return 0x03;
    case '4':
        return 0x0C;
    case 'q':
        return 0x04;
    case 'w':
        return 0x05;
    case 'e':
        return 0x06;
    case 'r':
        return 0x0D;
    case 'a':
        return 0x07;
    case 's':
        return 0x08;
    case 'd':
        return 0x09;
    case 'f':
        return 0x0E;
    case 'z':
        return 0x0A;
    case 'x':
        return 0x00;
    case 'c':
        return 0x0B;
    case 'v':
        return 0x0F;
    default:
        return BAD_KEY;
    }
}*/

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
    /*if (keypressed == '0') {
		return true;
	}
		
    uint8_t hexkey = key_to_hex(keypressed);
	keypressed = '0';

    // Send key press to emulator
    if (hexkey != BAD_KEY)
    {
		chip8.keypad[hexkey] = keyup ? KEY_UP : KEY_DOWN;
        return true;
    }

    return true;*/

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
    buttons_init();
	
	uart_init(500000);
	//uart_en_rx_int();
	/*while (keypressed == '0');
	for (volatile uint32_t i = 0; i < 400000; i++);*/

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
