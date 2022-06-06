#include "gpio.h"

void gpio_init(GPIO port) {
    uint32_t clk = GPIOA_CLK;

    switch (port) {
        case GPIOA:
            clk = GPIOA_CLK;
            break;
        case GPIOB:
            clk = GPIOB_CLK;
            break;
        case GPIOC:
            break;
        case GPIOD:
            break;
        case GPIOE:
            break;
        case GPIOF:
            break;
        case GPIOG:
            break;
    }

    RCC_APB2ENR |= clk;
    for (volatile int i = 0; i < 10; i++)
        ;
}
