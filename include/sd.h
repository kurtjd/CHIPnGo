#ifndef SD_H
#define SD_H

#include <stdint.h>
#include <stdbool.h>

bool sd_init(void);
void sd_write(uint8_t data);
uint8_t sd_read(void);
bool sd_inserted(void);

#endif
