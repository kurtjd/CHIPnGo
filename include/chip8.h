#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stdint.h>

#define ONE_SEC 1000

#define DISPLAY_WIDTH 128
#define DISPLAY_WIDTH_BYTES (DISPLAY_WIDTH / 8)
#define DISPLAY_HEIGHT 64

#define NUM_KEYS 16
#define NUM_REGISTERS 16
#define NUM_USER_FLAGS 16
#define NUM_QUIRKS 9
#define NUM_FONT_BYTES 80
#define NUM_BIG_FONT_BYTES 160

#define MAX_RAM 4096
#define STACK_SIZE 16

#define FONT_START_ADDR 0x0
#define BIG_FONT_START_ADDR (FONT_START_ADDR + NUM_FONT_BYTES)
#define SP_START_ADDR (BIG_FONT_START_ADDR + NUM_BIG_FONT_BYTES)

#define PC_START_ADDR_DEFAULT 0x200
#define CPU_FREQ_DEFAULT 0
#define REFRESH_FREQ_DEFAULT 30
#define TIMER_FREQ_DEFAULT 60

// The states each key of the keypad can be in.
typedef enum
{
    KEY_UP,
    KEY_DOWN,
    KEY_RELEASED
} CHIP8K;

typedef struct CHIP8
{
    // Represents random-access memory.
    uint8_t RAM[MAX_RAM];

    // Represents general-purpose 8-bit registers.
    uint8_t V[NUM_REGISTERS];

    // Program counter, stack pointer, and index 16-bit registers.
    uint16_t PC, SP, I;

    // Delay timer and sound timer 8-bit registers.
    uint8_t DT, ST;

    // A monochrome display. A pixel can be either only on or off, no color.
    uint8_t display[DISPLAY_HEIGHT][DISPLAY_WIDTH_BYTES];

    // Represents if a key is down, up, or released.
    CHIP8K keypad[NUM_KEYS];

    // Where the emulator begins reading instructions.
    uint16_t pc_start_addr;

    // These are used for handling CPU and timer speed.
    uint32_t cpu_freq;
    uint32_t timer_freq;
    uint32_t refresh_freq;
    uint32_t timer_max_cum;
    uint32_t cpu_max_cum;
    uint32_t cpu_cum;
    uint32_t sound_cum;
    uint32_t delay_cum;
    uint32_t refresh_max_cum;
    uint32_t refresh_cum;
    uint32_t prev_cycle_start;
    uint32_t cur_cycle_start;
    uint32_t total_cycle_time;

    // Flags for the various quirky behavior of S-CHIP
    /* Quirks:
        -0: RAM Initialization
        -1: 8xy6/8xyE
        -2: Fx55/Fx65
        -3: Bnnn
        -4: Big Sprite LORES
        -5: 00FE/00FF
        -6: Sprite Wrapping
        -7: Collision Enumeration
        -8: Collision with Bottom of Screen
    */
    bool quirks[NUM_QUIRKS];

    // Used to signal to main to update the display.
    bool display_updated;

    // Used to signal to main to produce sound.
    bool beep;

    // Used to signal to main to exit the program.
    bool exit;

    // Used to toggle between HI-RES and standard LO-RES modes.
    bool hires;
} CHIP8;

// Set some things to useful default values.
void chip8_init(CHIP8 *chip8, unsigned long cpu_freq, unsigned long timer_freq,
                unsigned long refresh_freq, uint16_t pc_start_addr,
                bool quirks[]);

// Reset the machine.
void chip8_reset(CHIP8 *chip8);

// Soft reset the machine (keep ROM and fonts loaded).
void chip8_soft_reset(CHIP8 *chip8);

// Sets the CPU frequency of the machine.
void chip8_set_cpu_freq(CHIP8 *chip8, unsigned long cpu_freq);

// Sets the timer frequency of the machine.
void chip8_set_timer_freq(CHIP8 *chip8, unsigned long timer_freq);

// Sets the refresh frequency of the machine.
void chip8_set_refresh_freq(CHIP8 *chip8, unsigned long refresh_freq);

// Load hexadecimal font into memory.
void chip8_load_font(CHIP8 *chip8);

// Loads a given ROM into memory.
bool chip8_load_rom(CHIP8 *chip8);
  
/* Performs a full cycle of the emulator including executing an instruction and
handling timers. Returns true if instruction was executed
or false if the CPU was sleeping. */
bool chip8_cycle(CHIP8 *chip8);

// Fetches, decodes, and executes the next instruction.
void chip8_execute(CHIP8 *chip8);

// Decrements delay and sound timers at specified frequency.
void chip8_handle_timers(CHIP8 *chip8);

// Updates the total cycle time since last call.
void chip8_update_elapsed_time(CHIP8 *chip8);

// Clears the keypad by setting all keys to up.
void chip8_reset_keypad(CHIP8 *chip8);

// Resets any key that was released to KEY_UP.
void chip8_reset_released_keys(CHIP8 *chip8);

// Clears the display by setting all pixels to off.
void chip8_reset_display(CHIP8 *chip8);

// Clears the RAM.
void chip8_reset_RAM(CHIP8 *chip8);

// Clears all registers.
void chip8_reset_registers(CHIP8 *chip8);

// Loads an instruction into memory.
void chip8_load_instr(CHIP8 *chip8, uint16_t instr);

// Performs a draw operation.
void chip8_draw(CHIP8 *chip8, uint8_t x, uint8_t y, uint8_t n);

// Scrolls the display in specified direction by num_pixels.
void chip8_scroll(CHIP8 *chip8, int xdir, int ydir, int num_pixels);

// Waits for a key to be released then stores that key in Vx.
void chip8_wait_key(CHIP8 *chip8, uint8_t x);

// Saves/loads user flags to/from disk.
bool chip8_handle_user_flags(CHIP8 *chip8, int num_flags, bool save);

// Skips the next instrtuction.
void chip8_skip_instr(CHIP8 *chip8);

// Sets a pixel on or off
void chip8_set_pixel(uint8_t buf[DISPLAY_HEIGHT][DISPLAY_WIDTH_BYTES], int x, int y, bool on);

// Gets a pixel's on/off status
bool chip8_get_pixel(uint8_t buf[DISPLAY_HEIGHT][DISPLAY_WIDTH_BYTES], int x, int y);

#endif
