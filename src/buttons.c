#include <stdbool.h>
#include "gpio.h"
#include "buttons.h"

#define BTN_LEFT_MODE  (1 << 19)
#define BTN_UP_MODE    (1 << 23)
#define BTN_DOWN_MODE  (1 << 27)
#define BTN_RIGHT_MODE (1 << 31)

#define BTN_A_MODE     (1 << 3)
#define BTN_B_MODE     (1 << 7)


void buttons_init(void) {
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
