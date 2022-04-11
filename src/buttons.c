#include <stdint.h>
#include <stdbool.h>
#include "buttons.h"
#include "clock.h"

#define BOUNCE_PERIOD 10

#define RCC         0x40021000
#define GPIOB       0x40010C00

#define GPIOB_CLK   0x08

#define RCC_APB2ENR (*((volatile uint32_t *)(RCC + 0x18)))
#define GPIOB_CRL   (*((volatile uint32_t *)(GPIOB + 0x00)))
#define GPIOB_CRH   (*((volatile uint32_t *)(GPIOB + 0x04)))
#define GPIOB_IDR   (*((volatile uint32_t *)(GPIOB + 0x08)))
#define GPIOB_ODR   (*((volatile uint32_t *)(GPIOB + 0x0C)))

#define BTN_LEFT_MODE  (1 << 19)
#define BTN_UP_MODE    (1 << 23)
#define BTN_DOWN_MODE  (1 << 27)
#define BTN_RIGHT_MODE (1 << 31)

#define BTN_A_MODE     (1 << 3)
#define BTN_B_MODE     (1 << 7)


void buttons_init(void) {
    RCC_APB2ENR |= GPIOB_CLK;

    // Clear registers since default state is not clear
    GPIOB_CRL = 0;
    GPIOB_CRH = 0;

    // Set inputs as internal pull-up
    GPIOB_CRL |= (BTN_LEFT_MODE | BTN_UP_MODE | BTN_DOWN_MODE | BTN_RIGHT_MODE);
    GPIOB_CRH |= (BTN_A_MODE | BTN_B_MODE);

    GPIOB_ODR |= 0x3F0; // Activate pull-up resistor
}

bool btn_pressed(enum Button btn) {
    return !(GPIOB_IDR & (1 << btn)); // Active-low logic
}

bool btn_released(enum Button btn) {
    // Keep track when each button was last pressed
    static bool btn_was_pressed[NUM_BUTTONS];
    /*static uint32_t last_press = 0;

    // Account for switch bounce
    if (clock_get() - last_press <= BOUNCE_PERIOD) {
        return false;
    }
    last_press = clock_get();*/

    // Figure out which switch we are dealing with for ease of use
    bool *was_pressed = &btn_was_pressed[btn - BTN_LEFT];

    /* If the switch is not currently pressed but was pressed recently,
       we know it has just been released */
    if (!btn_pressed(btn) && *was_pressed) {
        *was_pressed = false;
        return true;
    } else if (btn_pressed(btn)) {
        *was_pressed = true;
    }

    return false;
}
