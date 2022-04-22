#include <stdlib.h>
#include <string.h>
#include "clock.h"
#include "chip8.h"

void chip8_init(CHIP8 *chip8, unsigned long cpu_freq, unsigned long timer_freq,
                unsigned long refresh_freq, uint16_t pc_start_addr,
                bool quirks[])
{
    // Seed for the RND instruction.
    srand(69);

    for (int i = 0; i < NUM_QUIRKS; i++)
    {
        chip8->quirks[i] = quirks[i];
    }

    chip8_set_cpu_freq(chip8, cpu_freq);
    chip8_set_timer_freq(chip8, timer_freq);
    chip8_set_refresh_freq(chip8, refresh_freq);

    chip8->pc_start_addr = pc_start_addr;

    chip8_reset(chip8);
}

void chip8_reset(CHIP8 *chip8)
{
    chip8->PC = chip8->pc_start_addr;
    chip8->SP = SP_START_ADDR;
    chip8->I = 0x00;
    chip8->DT = 0;
    chip8->ST = 0;

    chip8->prev_cycle_start = clock_get();
    chip8->cur_cycle_start = clock_get();
    chip8->cpu_cum = 0;
    chip8->sound_cum = 0;
    chip8->delay_cum = 0;

    chip8->display_updated = false;
    chip8->beep = false;
    chip8->exit = false;
    chip8->hires = false;

    // S-CHIP did not initialize RAM (does it matter though?)
    if (!chip8->quirks[0])
    {
        chip8_reset_RAM(chip8);
    }

    chip8_reset_registers(chip8);
    chip8_reset_keypad(chip8);
    chip8_reset_display(chip8);
}

void chip8_soft_reset(CHIP8 *chip8)
{
    chip8_reset(chip8);
    chip8_load_font(chip8);
    chip8_load_rom(chip8);
}

void chip8_set_cpu_freq(CHIP8 *chip8, unsigned long cpu_freq)
{
    chip8->cpu_freq = cpu_freq;

    if (cpu_freq > 0)
    {
        chip8->cpu_max_cum = ONE_SEC / chip8->cpu_freq;
    }
}

void chip8_set_timer_freq(CHIP8 *chip8, unsigned long timer_freq)
{
    chip8->timer_freq = timer_freq;

    if (timer_freq > 0)
    {
        chip8->timer_max_cum = ONE_SEC / chip8->timer_freq;
    }
}

void chip8_set_refresh_freq(CHIP8 *chip8, unsigned long refresh_freq)
{
    chip8->refresh_freq = refresh_freq;

    if (refresh_freq > 0)
    {
        chip8->refresh_max_cum = ONE_SEC / chip8->refresh_freq;
    }
}

void chip8_load_font(CHIP8 *chip8)
{
    /* Characters are represented in memory as 5 bytes
    (while big characters are 10 bytes). Each byte represents a row and each
    bit represents a pixel. */

    static const uint8_t font_data[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80, // F

        // Big Hex (0-9):
        0xFF, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, // 0
        0x0C, 0x0C, 0x3C, 0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x3F, // 1
        0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, // 2
        0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 3
        0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, // 4
        0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 5
        0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, // 6
        0xFF, 0xFF, 0x03, 0x03, 0x0C, 0x0C, 0x30, 0x30, 0x30, 0x30, // 7
        0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, // 8
        0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 9

        /* Big Hex (A-F, which is not defined by original S-CHIP,
        but some programs assume they exist anyway) */
        0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, // A
        0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, // B
        0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xFF, 0xFF, // C
        0xFC, 0xFC, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFC, 0xFC, // D
        0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, // E
        0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0  // F
    };

    memcpy(chip8->RAM + FONT_START_ADDR, font_data, sizeof(font_data));
}

bool chip8_load_rom(CHIP8 *chip8)
{
    // Using a hardcoded ROM for now (Octopeg)
    static const uint8_t rom[] = {
        0x6E, 0x05, 0x65, 0x00, 0x6B, 0x06, 0x6A, 0x00, 0xA3, 0x0C, 0xDA, 0xB1, 0x7A, 0x04, 0x3A, 0x40, 
        0x12, 0x08, 0x7B, 0x02, 0x3B, 0x12, 0x12, 0x06, 0x6C, 0x20, 0x6D, 0x1F, 0xA3, 0x10, 0xDC, 0xD1, 
        0x22, 0xF6, 0x60, 0x00, 0x61, 0x00, 0xA3, 0x12, 0xD0, 0x11, 0x70, 0x08, 0xA3, 0x0E, 0xD0, 0x11, 
        0x60, 0x40, 0xF0, 0x15, 0xF0, 0x07, 0x30, 0x00, 0x12, 0x34, 0xC6, 0x0F, 0x67, 0x1E, 0x68, 0x01, 
        0x69, 0xFF, 0xA3, 0x0E, 0xD6, 0x71, 0xA3, 0x10, 0xDC, 0xD1, 0x60, 0x04, 0xE0, 0xA1, 0x7C, 0xFE, 
        0x60, 0x06, 0xE0, 0xA1, 0x7C, 0x02, 0x60, 0x3F, 0x8C, 0x02, 0xDC, 0xD1, 0xA3, 0x0E, 0xD6, 0x71, 
        0x86, 0x84, 0x87, 0x94, 0x60, 0x3F, 0x86, 0x02, 0x61, 0x1F, 0x87, 0x12, 0x47, 0x1F, 0x12, 0xAC, 
        0x46, 0x00, 0x68, 0x01, 0x46, 0x3F, 0x68, 0xFF, 0x47, 0x00, 0x69, 0x01, 0xD6, 0x71, 0x3F, 0x01, 
        0x12, 0xAA, 0x47, 0x1F, 0x12, 0xAA, 0x60, 0x05, 0x80, 0x75, 0x3F, 0x00, 0x12, 0xAA, 0x60, 0x01, 
        0xF0, 0x18, 0x80, 0x60, 0x61, 0xFC, 0x80, 0x12, 0xA3, 0x0C, 0xD0, 0x71, 0x60, 0xFE, 0x89, 0x03, 
        0x22, 0xF6, 0x75, 0x01, 0x22, 0xF6, 0x45, 0x60, 0x12, 0xDE, 0x12, 0x46, 0x69, 0xFF, 0x80, 0x60, 
        0x80, 0xC5, 0x3F, 0x01, 0x12, 0xCA, 0x61, 0x02, 0x80, 0x15, 0x3F, 0x01, 0x12, 0xE0, 0x80, 0x15, 
        0x3F, 0x01, 0x12, 0xEE, 0x80, 0x15, 0x3F, 0x01, 0x12, 0xE8, 0x60, 0x20, 0xF0, 0x18, 0xA3, 0x0E, 
        0x7E, 0xFF, 0x80, 0xE0, 0x80, 0x04, 0x61, 0x00, 0xD0, 0x11, 0x3E, 0x00, 0x12, 0x30, 0x12, 0xDE, 
        0x78, 0xFF, 0x48, 0xFE, 0x68, 0xFF, 0x12, 0xEE, 0x78, 0x01, 0x48, 0x02, 0x68, 0x01, 0x60, 0x04, 
        0xF0, 0x18, 0x69, 0xFF, 0x12, 0x70, 0xA3, 0x14, 0xF5, 0x33, 0xF2, 0x65, 0xF1, 0x29, 0x63, 0x37, 
        0x64, 0x00, 0xD3, 0x45, 0x73, 0x05, 0xF2, 0x29, 0xD3, 0x45, 0x00, 0xEE, 0xE0, 0x00, 0x80, 0x00, 
        0xFC, 0x00, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    memcpy(chip8->RAM + chip8->pc_start_addr, rom, sizeof(rom));
    return true;
}

bool chip8_cycle(CHIP8 *chip8)
{
    bool executed = false;
    chip8_update_elapsed_time(chip8);

    // Slow the CPU down to match given CPU frequency.
    chip8->cpu_cum += chip8->total_cycle_time;
    if (!chip8->cpu_freq || chip8->cpu_cum >= chip8->cpu_max_cum)
    {
        chip8->cpu_cum = 0;
        chip8_execute(chip8);
        executed = true;
    }

    chip8_handle_timers(chip8);
    return executed;
}

void chip8_execute(CHIP8 *chip8)
{
    /* Fetch */
    // The first and second byte of instruction respectively.
    uint8_t b1 = chip8->RAM[chip8->PC],
            b2 = chip8->RAM[chip8->PC + 1];

    /* Decode */
    // The code (first 4 bits) of instruction.
    uint8_t c = b1 >> 4;

    // The last 12 bits of instruction.
    uint16_t nnn = ((b1 & 0xF) << 8) | b2;

    // The last 4 bits of instruction.
    uint8_t n = b2 & 0xF;

    // The last 4 bits of first byte of instruction.
    uint8_t x = b1 & 0xF;

    // The first 4 bits of second byte of instruction.
    uint8_t y = b2 >> 4;

    // The last 8 bits of instruction.
    uint8_t kk = b2;

    /* Immediately set PC to next instruction
    after fetching and decoding the current one. */
    chip8->PC += 2;

    /* Execute */
    switch (c)
    {
    case 0x00:
        switch (b2)
        {
        /* HALT (0000)
           Halt the emulator. */
        case 0x00:
            chip8->PC -= 2;
            break;

        /* CLS (00E0)
           Clear the display. */
        case 0xE0:
            chip8_reset_display(chip8);
            break;

        /* RET (00EE):
           Return from a subroutine. */
        case 0xEE:
            chip8->PC = (chip8->RAM[chip8->SP] << 8);
            chip8->PC |= chip8->RAM[chip8->SP + 1];
            chip8->SP -= 2;
            break;

        /* SCRR (00FB) (S-CHIP Only):
           Scroll the display right by 4 pixels. */
        case 0xFB:
            chip8_scroll(chip8, 1, 0, 4);
            break;

        /* SCRL (00FC) (S-CHIP Only):
           Scroll the display left by 4 pixels. */
        case 0xFC:
            chip8_scroll(chip8, -1, 0, 4);
            break;

        /* EXIT (00FD) (S-CHIP Only):
           Exit the interpreter. */
        case 0xFD:
            chip8->exit = true;
            break;

        /* LORES (00FE) (S-CHIP Only):
           Disable HI-RES mode. */
        case 0xFE:
            chip8->hires = false;

            if (!chip8->quirks[5])
            {
                chip8_reset_display(chip8);
            }

            break;

        /* HIRES (00FF) (S-CHIP Only):
           Enable HI-RES mode. */
        case 0xFF:
            chip8->hires = true;

            if (!chip8->quirks[5])
            {
                chip8_reset_display(chip8);
            }

            break;

        default:
            switch (y)
            {
            /* SCRD (00Cn) (S-CHIP Only):
               Scroll the display down by n pixels. */
            case 0xC:
                chip8_scroll(chip8, 0, 1, n);
                break;

            /* SCRU (00Dn) (S-CHIP Only):
               Scroll the display up by n pixels. */
            case 0xD:
                chip8_scroll(chip8, 0, -1, n);
                break;
            }

            break;
        }

        break;

    /* JP addr (1nnn)
       Jump to location nnn. */
    case 0x01:
        chip8->PC = nnn;
        break;

    /* CALL addr (2nnn)
       Call subroutine at nnn. */
    case 0x02:
        chip8->SP += 2;
        chip8->RAM[chip8->SP] = chip8->PC >> 8;
        chip8->RAM[chip8->SP + 1] = chip8->PC & 0x00FF;
        chip8->PC = nnn;
        break;

    /* SE Vx, byte (3xkk)
       Skip next instruction if Vx = kk. */
    case 0x03:
        if (chip8->V[x] == kk)
        {
            chip8_skip_instr(chip8);
        }

        break;

    /* SNE Vx, byte (4xkk)
       Skip next instruction if Vx != kk. */
    case 0x04:
        if (chip8->V[x] != kk)
        {
            chip8_skip_instr(chip8);
        }

        break;

    case 0x05:
        switch (n)
        {
        /* SE Vx, Vy (5xy0)
           Skip next instruction if Vx = Vy. */
        case 0x0:
            if (chip8->V[x] == chip8->V[y])
            {
                chip8_skip_instr(chip8);
            }

            break;
        }

        break;

    /* LD Vx, byte (6xkk)
       Set Vx = kk. */
    case 0x06:
        chip8->V[x] = kk;
        break;

    /* ADD Vx, byte (7xkk)
       Set Vx = Vx + kk. */
    case 0x07:
        chip8->V[x] += kk;
        break;

    case 0x08:
        switch (n)
        {
        /* LD Vx, Vy (8xy0)
           Set Vx = Vy. */
        case 0x00:
            chip8->V[x] = chip8->V[y];
            break;

        /* OR Vx, Vy (8xy1)
           Set Vx = Vx OR Vy. */
        case 0x01:
            chip8->V[x] |= chip8->V[y];
            break;

        /* AND Vx, Vy (8xy2)
           Set Vx = Vx AND Vy. */
        case 0x02:
            chip8->V[x] &= chip8->V[y];
            break;

        /* XOR Vx, Vy (8xy3)
           Set Vx = Vx XOR Vy. */
        case 0x03:
            chip8->V[x] ^= chip8->V[y];
            break;

        /* ADD Vx, Vy (8xy4)
           Set Vx = Vx + Vy, set VF = carry. */
        case 0x04:
        {
            bool carry = ((chip8->V[x] + chip8->V[y]) > 0xFF);
            chip8->V[x] += chip8->V[y];
            chip8->V[0x0F] = carry;
            break;
        }

        /* SUB Vx, Vy (8xy5)
           Set Vx = Vx - Vy, set VF = NOT borrow. */
        case 0x05:
        {
            bool no_borrow = (chip8->V[x] >= chip8->V[y]);
            chip8->V[x] = chip8->V[x] - chip8->V[y];
            chip8->V[0x0F] = no_borrow;
            break;
        }

        /* SHR Vx {, Vy} (8xy6)
           Legacy: Set Vx = Vy SHR 1.
           S-CHIP: Set Vx = Vx SHR 1. */
        case 0x06:
            if (!chip8->quirks[1])
            {
                chip8->V[x] = chip8->V[y];
            }

            chip8->V[0x0F] = chip8->V[x] & 0x01;
            chip8->V[x] >>= 1;
            break;

        /* SUBN Vx, Vy (8xy7)
           Set Vx = Vy - Vx, set VF = NOT borrow. */
        case 0x07:
        {
            bool no_borrow = (chip8->V[y] >= chip8->V[x]);
            chip8->V[x] = chip8->V[y] - chip8->V[x];
            chip8->V[0x0F] = no_borrow;
            break;
        }

        /* SHL Vx {, Vy} (8xyE)
           Legacy: Set Vx = Vy SHL 1.
           S-CHIP: Set Vx = Vx SHL 1. */
        case 0x0E:
            if (!chip8->quirks[1])
            {
                chip8->V[x] = chip8->V[y];
            }

            chip8->V[0x0F] = (chip8->V[x] & 0x80) >> 7;
            chip8->V[x] <<= 1;
            break;
        }

        break;

    /* SNE Vx, Vy (9xy0)
       Skip next instruction if Vx != Vy. */
    case 0x09:
        if (chip8->V[x] != chip8->V[y])
        {
            chip8_skip_instr(chip8);
        }

        break;

    /* LD I, addr (Annn)
       Set I = nnn. */
    case 0x0A:
        chip8->I = nnn;
        break;

    /* JP V0, addr (Bnnn)
       Legacy: Jump to location nnn + V0.
       S-CHIP: Jump to location nnn + Vx. */
    case 0x0B:
        chip8->PC = (!chip8->quirks[3]) ? chip8->V[0] + nnn : chip8->V[x] + nnn;
        break;

    /* RND Vx, byte (Cxkk)
       Set Vx = random byte AND kk. */
    case 0x0C:
        chip8->V[x] = (rand() % 0x100) & kk;
        break;

    /* DRW Vx, Vy, n (Dxyn):
       Legacy: Display n-byte sprite starting at memory location I at (Vx, Vy),
       set VF = collision.
       S-CHIP: If hires=false: If n=0, display 8x16 sprite. Else:
       Same as Legacy. If hires=true: Same as Legacy, except
       set VF = num rows collision. If n=0: Display 16x16 sprite starting at
       memory location I at (Vx, Vy), set VF = num rows collision. */
    case 0x0D:
        chip8_draw(chip8, chip8->V[x], chip8->V[y], n);
        break;

    case 0x0E:
        switch (b2)
        {
        /* SKP Vx (Ex9E)
           Skip next instruction if key with the value of Vx is pressed. */
        case 0x9E:
            if (chip8->keypad[chip8->V[x]] == KEY_DOWN)
            {
                chip8_skip_instr(chip8);
            }

            break;

        /* SKNP Vx (ExA1)
           Skip next instruction if key with the value of Vx is not pressed. */
        case 0xA1:
            if (chip8->keypad[chip8->V[x]] == KEY_UP)
            {
                chip8_skip_instr(chip8);
            }

            break;
        }

        break;

    case 0x0F:
        switch (b2)
        {
        /* LD Vx, DT (Fx07)
           Set Vx = delay timer value. */
        case 0x07:
            chip8->V[x] = chip8->DT;
            break;

        /* LD Vx, K (Fx0A)
           Wait for a key press, store the value of the key in Vx. */
        case 0x0A:
        {
            chip8_wait_key(chip8, x);
            break;
        }

        /* LD DT, Vx (Fx15)
           Set delay timer = Vx. */
        case 0x15:
            chip8->DT = chip8->V[x];
            break;

        /* LD ST, Vx (Fx18)
           Set sound timer = Vx. */
        case 0x18:
            chip8->ST = chip8->V[x];
            break;

        /* ADD I, Vx (Fx1E):
           Set I = I + Vx. */
        case 0x1E:
            chip8->I += chip8->V[x];
            break;

        /* LD F, Vx (Fx29)
           Set I = location of 5-byte sprite for digit Vx. */
        case 0x29:
            chip8->I = FONT_START_ADDR + (chip8->V[x] * 0x05);
            break;

        /* LD HF, Vx (Fx30) (S-CHIP Only)
           Set I = location of 10-byte sprite for digit Vx. */
        case 0x30:
            chip8->I = BIG_FONT_START_ADDR + (chip8->V[x] * 0x0A);
            break;

        /* LD B, Vx (Fx33)
           Store BCD representation of Vx in memory locations:
           I, I+1, and I+2. */
        case 0x33:
            chip8->RAM[chip8->I] = (chip8->V[x] / 100) % 10;
            chip8->RAM[chip8->I + 1] = (chip8->V[x] / 10) % 10;
            chip8->RAM[chip8->I + 2] = chip8->V[x] % 10;
            break;

        /* LD [I], Vx (Fx55)
           Store registers V0 through Vx in memory starting at location I.
           Legacy: Set I=I+x+1 */
        case 0x55:
            for (int r = 0; r <= x; r++)
            {
                chip8->RAM[chip8->I + r] = chip8->V[r];
            }

            if (!chip8->quirks[2])
            {
                chip8->I += (x + 1);
            }

            break;

        /* LD Vx, [I] (Fx65)
           Read registers V0 through Vx from memory starting at location I.
           Legacy: Set I=I+x+1 */
        case 0x65:
            for (int r = 0; r <= x; r++)
            {
                chip8->V[r] = chip8->RAM[chip8->I + r];
            }

            if (!chip8->quirks[2])
            {
                chip8->I += (x + 1);
            }

            break;

        /* LD uflags_disk, V0..Vx (Fx75) (S-CHIP Only)
           Save user flags to disk. */
        case 0x75:
            /*if (!chip8_handle_user_flags(chip8, x + 1, true))
            {
                fprintf(stderr, "Unable to save user flags to %s\n",
                        chip8->UF_path);
            }*/

            break;

        /* LD V0..Vx, uflags_disk (Fx85) (S-CHIP Only)
           Load user flags from disk. */
        case 0x85:
            /*if (!chip8_handle_user_flags(chip8, x + 1, false))
            {
                fprintf(stderr, "Unable to load user flags from %s\n",
                        chip8->UF_path);
            }*/

            break;
        }

        break;
    }

    // Any key that was released previous frame gets turned off.
    chip8_reset_released_keys(chip8);
}

void chip8_handle_timers(CHIP8 *chip8)
{
    // Delay
    if (chip8->DT > 0)
    {
        chip8->delay_cum += chip8->total_cycle_time;

        if (!chip8->timer_freq || chip8->delay_cum >= chip8->timer_max_cum)
        {
            chip8->DT--;
            chip8->delay_cum = 0;
        }
    }

    // Sound
    if (chip8->ST > 0)
    {
        chip8->beep = true;
        chip8->sound_cum += chip8->total_cycle_time;

        if (!chip8->timer_freq || chip8->sound_cum >= chip8->timer_max_cum)
        {
            chip8->ST--;
            chip8->sound_cum = 0;
        }
    }
    else
    {
        chip8->beep = false;
    }

    // Screen Refresh
    chip8->display_updated = false;
    chip8->refresh_cum += chip8->total_cycle_time;
    if (!chip8->refresh_freq || chip8->refresh_cum >= chip8->refresh_max_cum)
    {
        chip8->display_updated = true;
        chip8->refresh_cum = 0;
    }
}

void chip8_update_elapsed_time(CHIP8 *chip8)
{
    chip8->prev_cycle_start = chip8->cur_cycle_start;
    chip8->cur_cycle_start = clock_get();
    chip8->total_cycle_time = chip8->cur_cycle_start - chip8->prev_cycle_start;
}

void chip8_reset_keypad(CHIP8 *chip8)
{
    for (int k = 0; k < NUM_KEYS; k++)
    {
        chip8->keypad[k] = KEY_UP;
    }
}

void chip8_reset_released_keys(CHIP8 *chip8)
{
    for (int k = 0; k < NUM_KEYS; k++)
    {
        if (chip8->keypad[k] == KEY_RELEASED)
        {
            chip8->keypad[k] = KEY_UP;
        }
    }
}

void chip8_reset_display(CHIP8 *chip8)
{
    for (int y = 0; y < DISPLAY_HEIGHT; y++)
    {
        for (int x = 0; x < DISPLAY_WIDTH_BYTES; x++)
        {
            chip8->display[y][x] = 0;
        }
    }
}

void chip8_reset_RAM(CHIP8 *chip8)
{
    for (int i = 0; i < MAX_RAM; i++)
    {
        chip8->RAM[i] = 0x00;
    }
}

void chip8_reset_registers(CHIP8 *chip8)
{
    for (int i = 0; i < NUM_REGISTERS; i++)
    {
        chip8->V[i] = 0x00;
    }
}

void chip8_load_instr(CHIP8 *chip8, uint16_t instr)
{
    chip8->RAM[chip8->pc_start_addr] = instr >> 8;
    chip8->RAM[chip8->pc_start_addr + 1] = instr & 0x00FF;
}

void chip8_draw(CHIP8 *chip8, uint8_t x, uint8_t y, uint8_t n)
{
    // This function is ugly and could probably use some refactoring...

    chip8->V[0x0F] = 0;
    int rows;

    /* n==0 only has signifigance in S-CHIP mode,
    otherwise nothing should be drawn. */
    if (n == 0)
    {
        /* Draw a 32-byte (16x16) sprite in hires or
        a 16-byte (8x16) sprite in lores. */
        n = (chip8->hires || !chip8->quirks[4]) ? 32 : 16;
    }

    if (chip8->hires && chip8->quirks[8])
    {
        rows = (n == 32) ? 16 : n;
        chip8->V[0x0F] += ((y + rows) - (DISPLAY_HEIGHT - 1));
    }
    else
    {
        rows = n;
    }

    // Allow out-of-bound sprite to wrap-around.
    if (!chip8->quirks[6])
    {
        y %= DISPLAY_HEIGHT;
        x %= DISPLAY_WIDTH;
    }

    bool prev_byte_collide = false;

    for (int i = 0; i < n; i++)
    {
        bool collide_row = false;

        for (int j = 0; j < 8; j++)
        {
            /* For big sprites, every odd byte needs to be drawn on the same
            row as the previous byte. This is achieved through integer division
            truncation. The odd byte also needs to be drawn 8 pixels to the
            right of the previous byte. */
            unsigned y_start = (n == 32) ? (i / 2) : i;
            unsigned x_start = (n == 32 && (i % 2 != 0)) ? (j + 8) : j;

            /* Now we have to scale the display if we are in lo-res mode
            by basically drawing each pixel twice. */
            int scale = chip8->hires ? 1 : 2;
            for (int h = 0; h < scale; h++)
            {
                for (int k = 0; k < scale; k++)
                {
                    int disp_x = (x * scale) + (x_start * scale) + k;
                    int disp_y = (y * scale) + (y_start * scale) + h;
                    if (disp_x >= DISPLAY_WIDTH || disp_y >= DISPLAY_HEIGHT)
                    {
                        break;
                    }

                    bool pixel_on = false;
                    bool bit = false;
                    bool collide = false;

                    /* Get the pixel the loop is on and the corresponding bit
                    and XOR them onto display. If a pixel is erased, set the VF
                    register to 1. */
                    pixel_on = chip8_get_pixel(chip8->display, disp_x, disp_y);
                    bit = (chip8->RAM[chip8->I + i] >> (7 - j)) & 0x01;
                    collide = pixel_on && bit;
                    chip8_set_pixel(chip8->display, disp_x, disp_y, pixel_on ^ bit);

                    if (collide)
                    {
                        if (chip8->hires && chip8->quirks[7])
                        {
                            if (!collide_row)
                            {
                                if (n <= 16 || (((i % 2 == 0) && n == 32) ||
                                                !prev_byte_collide))
                                {
                                    chip8->V[0x0F]++;
                                    collide_row = true;
                                }
                            }
                        }
                        else
                        {
                            chip8->V[0x0F] = 1;
                        }
                    }
                }
            }
        }

        prev_byte_collide = collide_row;
    }
}

void chip8_scroll(CHIP8 *chip8, int xdir, int ydir, int num_pixels)
{
    int x_start = 0;
    int x_end = DISPLAY_WIDTH;
    int y_start = 0;
    int y_end = DISPLAY_HEIGHT;

    if (xdir == 1)
    {
        x_end = DISPLAY_WIDTH - num_pixels;
    }
    else if (xdir == -1)
    {
        x_start = num_pixels;
    }
    if (ydir == 1)
    {
        y_end = DISPLAY_HEIGHT - num_pixels;
    }
    else if (ydir == -1)
    {
        y_start = num_pixels;
    }

    // Create updated display buffers.
    uint8_t disp_buf[DISPLAY_HEIGHT][DISPLAY_WIDTH_BYTES];
    for (int i = 0; i < DISPLAY_HEIGHT; i++)
    {
        for (int j = 0; j < DISPLAY_WIDTH_BYTES; j++)
        {
            disp_buf[i][j] = 0;
        }
    }

    for (int y = y_start; y < y_end; y++)
    {
        for (int x = x_start; x < x_end; x++)
        {
            int buf_y = y + (ydir * num_pixels);
            int buf_x = x + (xdir * num_pixels);

            chip8_set_pixel(disp_buf, buf_x, buf_y, chip8_get_pixel(chip8->display, x, y));
        }
    }

    // Copy updated display buffer into actual display.
    for (int y = 0; y < DISPLAY_HEIGHT; y++)
    {
        for (int x = 0; x < DISPLAY_WIDTH_BYTES; x++)
        {
            chip8->display[y][x] = disp_buf[y][x];
        }
    }
}

void chip8_wait_key(CHIP8 *chip8, uint8_t x)
{
    bool key_released = false;

    for (int i = 0; i < NUM_KEYS; i++)
    {
        if (chip8->keypad[i] == KEY_RELEASED)
        {
            chip8->V[x] = i;
            key_released = true;
            break;
        }
    }

    if (!key_released)
    {
        chip8->PC -= 2;
    }
}

bool chip8_handle_user_flags(CHIP8 *chip8, int num_flags, bool save)
{
    // Maybe use eeprom for this
    return true;
}

void chip8_skip_instr(CHIP8 *chip8)
{
    chip8->PC += 2;
}

void chip8_set_pixel(uint8_t buf[DISPLAY_HEIGHT][DISPLAY_WIDTH_BYTES], int x, int y, bool on)
{
    int byte = (x / 8);
    int bit = x % 8;

    if (on)
    {
        buf[y][byte] |= (1 << (7 - bit));
    }
    else
    {
        buf[y][byte] &= ~(1 << (7 - bit));
    }
}

bool chip8_get_pixel(uint8_t buf[DISPLAY_HEIGHT][DISPLAY_WIDTH_BYTES], int x, int y)
{
    int byte = (x / 8);
    int bit = x % 8;

    return ((buf[y][byte]) & (1 << (7 - bit)));
}
