#include <stdint.h>
#include "sysclk.h"

#define MHz 1000000

#define DEAULT_CLK_SPEED (8 * MHz)

#define RCC         0x40021000
#define FLASH       0x40022000

#define RCC_CR      (*((volatile uint32_t *)(RCC + 0x00)))
#define RCC_CFGR    (*((volatile uint32_t *)(RCC + 0x04)))

#define FLASH_ACR   (*((volatile uint32_t *)(FLASH + 0x00)))

long CLOCK_SPEED = DEAULT_CLK_SPEED;
long APB1_CLOCK_SPEED = DEAULT_CLK_SPEED;
long APB2_CLOCK_SPEED = DEAULT_CLK_SPEED;
long ADC_CLOCK_SPEED = DEAULT_CLK_SPEED;
long AHB_CLOCK_SPEED = DEAULT_CLK_SPEED;

void set_sysclk(int mhz) {
    if (mhz > 72) {
        mhz = 72;
    } else if (mhz < 16) {
        mhz = 16;
    }

    // Make mhz a multiple of 8
    int mult = mhz / 8;
    mhz = mult * 8;

    RCC_CR |= (1 << 16);                // Enable HSE
    while (!(RCC_CR & (1 << 17)));      // Wait for HSE

    FLASH_ACR |= (1 << 4);              // Enable prefetch buffer for flash

    // Set flash access wait states/latency based on speed
    if (mhz > 24 && mhz <= 48) {
        FLASH_ACR |= 0x01;
    } else if (mhz > 48) {
        FLASH_ACR |= 0x02;
    }

    RCC_CFGR &= ~(1 << 17);             // Don't divide HSE clk
    RCC_CFGR |= (1 << 16);              // Set HSE as PLL clock

    if (mult >= 3) {
        RCC_CFGR |= ((mult - 2) << 18); // Set PLL multiplier according to speed
    } else {
        RCC_CFGR &= ~(0x0F << 18);      // Set PLL multiplier 2
    }

    RCC_CFGR &= ~(0x0F << 4);           // AHB no divide
    RCC_CFGR &= ~(0x03 << 14);          // ADC no divide
    RCC_CFGR &= ~(0x07 << 11);          // APB2 no divide

    // If desired speed is over 36 mhz, set APB1 divider to 2
    if (mhz > 36) {
        RCC_CFGR |= (4 << 8);
        APB1_CLOCK_SPEED = (mhz / 2) * MHz;
    } else {
        RCC_CFGR &= ~(0x07 << 8);
        APB1_CLOCK_SPEED = mhz * MHz;
    }

    // Handle USB prescaler maybe in the future?

    RCC_CR |= (1 << 24);                // Enable PLL
    while (!(RCC_CR & (1 << 25)));      // Wait for PLL

    RCC_CFGR |= 0x02;                   // Set PLL as sysclk
    while (!(RCC_CFGR & (1 << 3)));     // Wait for sysclk

    CLOCK_SPEED = mhz * MHz;
    APB2_CLOCK_SPEED = CLOCK_SPEED;
    ADC_CLOCK_SPEED = CLOCK_SPEED;
    AHB_CLOCK_SPEED = CLOCK_SPEED;
}

