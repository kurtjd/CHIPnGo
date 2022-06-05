#include <stdint.h>
#include "gpio.h"
#include "clock.h"
#include "sysclk.h"

#define NVIC        0xE000E100
#define NVIC_ISER0  (*((volatile uint32_t *)(NVIC + 0x00)))
#define TIM2_NVIC (1 << 28)

#define TIM2 0x40000000
#define TIM2_CR1  (*((volatile uint32_t *)(TIM2 + 0x00)))
#define TIM2_DIER  (*((volatile uint32_t *)(TIM2 + 0x0C)))
#define TIM2_SR  (*((volatile uint32_t *)(TIM2 + 0x10)))
#define TIM2_CNT  (*((volatile uint32_t *)(TIM2 + 0x24)))
#define TIM2_PSC  (*((volatile uint32_t *)(TIM2 + 0x28)))
#define TIM2_ARR  (*((volatile uint32_t *)(TIM2 + 0x2C)))
#define TIM2EN 1

static volatile uint32_t elapsed_ms = 0;

void TIM2_IRQHandler(void) {
	elapsed_ms += 1000; // Since this ISR is called every second
	TIM2_SR &= ~1; // Clear interrupt
}

void clock_start(void) {
	RCC_APB1ENR |= TIM2EN;
	NVIC_ISER0 |= TIM2_NVIC;
	TIM2_DIER |= 1;

	/* We want overflow to happen every second.
	 * Since the actual clock speed in MHz wouldn't fit in the ARR buffer,
	 * We divide the clock by 1000. That way, we can fit the clock speed
	 * in the ARR buffer and it overflows every second. */
	TIM2_PSC = 1000;
	TIM2_ARR = APB1_CLOCK_SPEED / 1000;

	TIM2_CR1 |= 1; // Finally enable timer
}

uint32_t clock_get(void) {
	return elapsed_ms + (TIM2_CNT / (APB1_CLOCK_SPEED / 1000000));
}
