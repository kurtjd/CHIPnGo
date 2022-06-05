#include <stdint.h>
#include "led.h"

#define RCC         0x40021000
#define GPIOC       0x40011000

#define GPIOC_CLK   0x10
#define LED_PIN     0x2000

#define RCC_APB2ENR (*((volatile uint32_t *)(RCC + 0x18)))
#define GPIOC_CRH   (*((volatile uint32_t *)(GPIOC + 0x04)))
#define GPIOC_ODR   (*((volatile uint32_t *)(GPIOC + 0x0C)))

void led_enable(void) {
    RCC_APB2ENR |= GPIOC_CLK;
    GPIOC_CRH &= 0xFF0FFFFF;
    GPIOC_CRH |= 0x00200000;
    led_off();
}

void led_on(void) {
    GPIOC_ODR &= ~(LED_PIN);
}

void led_off(void) {
    GPIOC_ODR |= LED_PIN;
}

void led_toggle(void) {
    GPIOC_ODR ^= LED_PIN;
}
