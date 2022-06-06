#ifndef SD_H
#define SD_H

#include <stdbool.h>
#include <stdint.h>

#define SD_BLOCK_SIZE 512

bool sd_init(void);
bool sd_inserted(void);
void sd_read_block(uint32_t addr, uint8_t *buffer);
void sd_read_blocks(uint32_t addr, uint8_t *buffer, int num_blocks);
bool sd_write_block(uint32_t addr, const uint8_t *buffer);

#endif
