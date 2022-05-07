#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

void display_init(void);
void display_send_data(uint8_t data);
void display_send_cmd(uint8_t cmd);
void display_clear(void);
void display_draw(uint8_t buf[64][16]);

void display_test(void);

#endif
