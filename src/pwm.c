/*
 * This is a basic PWM driver specifically for Timer 3, Channel 4
 * Thus, according to the pin-out, this maps to Pin B1
 * Essentially, this PWM driver is configured only for single-frequency sound
 * Therefore, the duty cycle is always 50% and the frequency is set only once
 * Two functions exist to either start or stop the pulse
 */
#include "gpio.h"
#include "sysclk.h"
#include "pwm.h"

#define TIM3_START  0x40000400
#define TIM3_CR1    (*((volatile uint32_t *)(TIM3_START + 0x00)))
#define TIM3_PSC    (*((volatile uint32_t *)(TIM3_START + 0x28)))
#define TIM3_ARR    (*((volatile uint32_t *)(TIM3_START + 0x2C)))
#define TIM3_CCMR2  (*((volatile uint32_t *)(TIM3_START + 0x1C)))
#define TIM3_CCR4   (*((volatile uint32_t *)(TIM3_START + 0x40)))
#define TIM3_CCER   (*((volatile uint32_t *)(TIM3_START + 0x20)))

#define TIM3EN 0x02
#define GPIOB_TIM3_MODE (1 << 5)
#define GPIOB_TIM3_CONF (1 << 7)
#define PWM_MODE_2      (0x07 << 12)
#define CC4E            (1 << 12)

#define PSC 10 // Chosen so the ARR wouldn't overflow its register and just nice


// Configure port b pin 1 as alternate function output push-pull
static void _gpio_init(void) {
    GPIOB_CRL |= GPIOB_TIM3_MODE;
    GPIOB_CRL &= ~(0x03 << 6);
    GPIOB_CRL |= GPIOB_TIM3_CONF;
}

void pwm_init(int freq) {
    _gpio_init();

    // Enable timer 3 clock
    RCC_APB1ENR |= TIM3EN;

    // Put timer 3 into PWM mode 2 (active when timer count above CCR4)
    // Honestly for this case doesn't really matter if mode 1 or 2
    TIM3_CCMR2 |= PWM_MODE_2;

    // Enable channel 4 of timer 3
    TIM3_CCER |= CC4E;

    // Set prescaler
    TIM3_PSC = PSC;

    // Calculate the ARR value based on the given freq and a set prescaler
    TIM3_ARR = APB1_CLOCK_SPEED / (freq * PSC);

    // Set the duty cycle (always 50%)
    TIM3_CCR4 = (TIM3_ARR / 2);
}

void pwm_start(void) {
    // Enable timer 3
    TIM3_CR1 |= 1;
}

void pwm_stop(void) {
    // Disable timer 3
    TIM3_CR1 &= ~1;
}
