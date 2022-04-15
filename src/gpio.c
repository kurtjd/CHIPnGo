#include "gpio.h"

void gpio_init(GPIO port) {
    switch (port) {
    case GPIOA:
        break;
    case GPIOB:
        RCC_APB2ENR |= GPIOB_CLK;

        // Clear registers since default state is not clear
        GPIOB_CRL = 0;
        GPIOB_CRH = 0;
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
}
