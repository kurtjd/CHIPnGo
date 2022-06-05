#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sysclk.h"
#include "clock.h"
#include "gpio.h"
#include "uart.h"
#include "sd.h"
#include "display.h"
#include "buttons.h"
#include "pwm.h"
#include "delay.h"
#include "chip8.h"
#include "led.h"

#define MAX_ROMS 25

// Emulator (TODO: Put this all in struct)
CHIP8 chip8;
uint8_t metadata[SD_BLOCK_SIZE] = {0};
char title[11];
uint32_t cpu_freq = CPU_FREQ_DEFAULT;
uint32_t timer_freq = TIMER_FREQ_DEFAULT;
uint32_t refresh_freq = REFRESH_FREQ_DEFAULT;
bool quirks[NUM_QUIRKS] = {0, 0, 0, 0, 0, 0, 0, 0};
uint16_t BTN_LEFT_MAP = 0x90;
uint16_t BTN_RIGHT_MAP = 0x240;
uint16_t BTN_UP_MAP = 0x20;
uint16_t BTN_DOWN_MAP = 0x100;
uint16_t BTN_A_MAP = 0x00;
uint16_t BTN_B_MAP = 0x40;
bool play_sound = false;
int rom_num = 0;


// A basic splash screen that waits for user to press A
void show_splash(void) {
    display_print(37, 4, "CHIP N GO");
    //display_print(20, 4, "PRESS A TO PLAY");
    //display_print(20, 7, "CREATED BY KURT");

    for (int i = 0; i < 10; i++) {
        pwm_start();
        delay(100);
        pwm_stop();
        delay(100);
    }

    display_clear();

    //while (!btn_released(BTN_A));
    //delay(2000);
}

void btn_to_key(uint16_t btn_map, CHIP8K action) {
    for (int i = 0; i < 16; i++) {
        if (btn_map & 1)
            chip8.keypad[i] = action;
        btn_map >>= 1;
    }
}

void parse_quirks(uint8_t q) {
    for (int i = 0; i < 8; i++) {
        quirks[i] = q & 1;
        q >>= 1;
    }
}


// Set up the emulator to begin running.
bool init_emulator(void)
{
    chip8_init(&chip8, cpu_freq, timer_freq, refresh_freq, PC_START_ADDR_DEFAULT,
                quirks, metadata, rom_num);
    chip8_load_font(&chip8);

    return true;
}

void load_rom(int rom_num) {
    uint32_t start_sector = rom_num * 8;
    sd_read_block(start_sector, metadata);
    sd_read_blocks(start_sector + 1, chip8.RAM + PC_START_ADDR_DEFAULT, 7);
}

bool process_metadata(void) {
    if (metadata[0] == 0xC8) {
        strcpy(title, (char *)(&metadata[1]));

        cpu_freq = (metadata[12] << 24) | (metadata[13] << 16) | (metadata[14] << 8) | (metadata[15]);
        timer_freq = metadata[16];
        refresh_freq = metadata[17];

        parse_quirks(metadata[18]);

        BTN_LEFT_MAP = (metadata[19] << 8) | (metadata[20]);
        BTN_RIGHT_MAP = (metadata[21] << 8) | (metadata[22]);
        BTN_UP_MAP = (metadata[23] << 8) | (metadata[24]);
        BTN_DOWN_MAP = (metadata[25] << 8) | (metadata[26]);
        BTN_A_MAP = (metadata[27] << 8) | (metadata[28]);
        BTN_B_MAP = (metadata[29] << 8) | (metadata[30]);

        return true;
    }

    return false;
}

// Very slow... but functional for now
bool seek_rom(int dir) {
    int attempts = 0;

    do {
        if (rom_num >= MAX_ROMS)
            rom_num = 0;
        else if (rom_num < 0)
            rom_num = MAX_ROMS - 1;

        load_rom(rom_num);
        rom_num += dir;

        attempts++;
    } while (attempts <= MAX_ROMS && !process_metadata());

    if (attempts <= MAX_ROMS) {
        rom_num -= dir;
        return true;
    }

    // No ROM exists if we attempted to find a ROM more than MAX_ROMS times
    return false;
}

void select_rom(void) {
    int scan_dir = 1;
    bool rom_exists = seek_rom(scan_dir);

    while (rom_exists) {
        display_clear();
        display_print(36 + ((10 - strlen(title)) * 2), 3, title);
        display_print(2, 4, "<                   >");
        display_print(19, 5, "PRESS A TO PLAY");

        scan_dir = 0;
        while (!scan_dir) {
            if (btn_released(BTN_A)) {
                pwm_start();
                //delay(500);
                for (volatile uint32_t i = 0; i < 5000000; i++);
                pwm_stop();
                return;
            } else if (btn_released(BTN_RIGHT))
                scan_dir = 1;
            else if (btn_released(BTN_LEFT))
                scan_dir = -1;
        }
        rom_num += scan_dir;
        
        pwm_start();
        //delay(1);
        for (volatile uint32_t i = 0; i < 100000; i++);
        pwm_stop();

        rom_exists = seek_rom(scan_dir);
    }

    // No ROM exists on game cartridge
    // Just hang since no way to recover
    display_clear();
    display_print(25, 4, "ROM NOT FOUND");
    while(1);
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
    if (btn_pressed(BTN_LEFT))
        btn_to_key(BTN_LEFT_MAP, KEY_DOWN);
    if (btn_pressed(BTN_UP))
        btn_to_key(BTN_UP_MAP, KEY_DOWN);
    if (btn_pressed(BTN_DOWN))
        btn_to_key(BTN_DOWN_MAP, KEY_DOWN);
    if (btn_pressed(BTN_RIGHT))
        btn_to_key(BTN_RIGHT_MAP, KEY_DOWN);
    if (btn_pressed(BTN_A))
        btn_to_key(BTN_A_MAP, KEY_DOWN);
    if (btn_pressed(BTN_B))
        btn_to_key(BTN_B_MAP, KEY_DOWN);

    if (btn_released(BTN_LEFT))
        btn_to_key(BTN_LEFT_MAP, KEY_RELEASED);
    if (btn_released(BTN_UP))
        btn_to_key(BTN_UP_MAP, KEY_RELEASED);
    if (btn_released(BTN_DOWN))
        btn_to_key(BTN_DOWN_MAP, KEY_RELEASED);
    if (btn_released(BTN_RIGHT))
        btn_to_key(BTN_RIGHT_MAP, KEY_RELEASED);
    if (btn_released(BTN_A))
        btn_to_key(BTN_A_MAP, KEY_RELEASED);
    if (btn_released(BTN_B))
        btn_to_key(BTN_B_MAP, KEY_RELEASED);
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

void handle_sd() {
    // Wait for user to insert SD card
    if (!sd_inserted()) {
        display_print(2, 4, "INSERT GAME CARTRIDGE");
        while(!sd_inserted());

        // Wait briefly for SD to be fully inserted
        display_clear();
        delay(100);
    }

    // Can't recover so just hang and tell the user to restart
    if (!sd_init()) {
        display_print(5, 3, "GAME CARTRIDGE ERROR");
        display_print(22, 4, "PLEASE RESTART");
        while(1);
    }
}

int main(void)
{
    set_sysclk(72);

    gpio_init(GPIOA);
    gpio_init(GPIOB);
    led_enable();

    pwm_init(880);

    display_init();
    show_splash();
    handle_sd();

    clock_start();
    buttons_init();
    select_rom();
    init_emulator();
    //clock_start();

    while (1)
    {
        handle_input();
        chip8_cycle(&chip8);
        handle_sound();
        handle_display();
    }

    return 0;
}
