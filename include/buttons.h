#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>

#define NUM_BUTTONS 6

enum Button {
    BTN_DOWN = 4,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_UP,
    BTN_B = 10,
    BTN_A = 11
};

void buttons_init(void);
bool btn_pressed(enum Button btn);
bool btn_released(enum Button btn);

#endif