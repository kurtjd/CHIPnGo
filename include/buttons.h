#ifndef BUTTONS_H
#define BUTTONS_H

#define NUM_BUTTONS 6

enum Button {
    BTN_LEFT = 4,
    BTN_DOWN,
    BTN_UP,
    BTN_RIGHT,
    BTN_A,
    BTN_B
};

void buttons_init(void);
bool btn_pressed(enum Button btn);
bool btn_released(enum Button btn);

#endif