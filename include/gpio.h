#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#define RCC 0x40021000
#define RCC_APB2ENR (*((volatile uint32_t *)(RCC + 0x18)))
#define RCC_APB1ENR (*((volatile uint32_t *)(RCC + 0x1C)))

#define GPIOB_CLK 0x08
#define GPIOB_START 0x40010C00
#define GPIOB_CRL (*((volatile uint32_t *)(GPIOB_START + 0x00)))
#define GPIOB_CRH (*((volatile uint32_t *)(GPIOB_START + 0x04)))
#define GPIOB_IDR (*((volatile uint32_t *)(GPIOB_START + 0x08)))
#define GPIOB_ODR (*((volatile uint32_t *)(GPIOB_START + 0x0C)))

#define GPIOA_CLK 0x04
#define GPIOA_START 0x40010800
#define GPIOA_CRL (*((volatile uint32_t *)(GPIOA_START + 0x00)))
#define GPIOA_CRH (*((volatile uint32_t *)(GPIOA_START + 0x04)))
#define GPIOA_IDR (*((volatile uint32_t *)(GPIOA_START + 0x08)))
#define GPIOA_ODR (*((volatile uint32_t *)(GPIOA_START + 0x0C)))

typedef enum GPIO {
    GPIOA,
    GPIOB,
    GPIOC,
    GPIOD,
    GPIOE,
    GPIOF,
    GPIOG
} GPIO;

void gpio_init(GPIO port);

#endif
