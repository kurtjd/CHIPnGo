#ifndef SD_H
#define SD_H

#include <stdint.h>
#include <stdbool.h>

#define SD_BLOCK_SIZE 512

bool sd_init(void);
void sd_write(uint8_t data);
uint8_t sd_read(void);
bool sd_inserted(void);
void sd_read_block(uint16_t addr, uint8_t *buffer);

#endif
