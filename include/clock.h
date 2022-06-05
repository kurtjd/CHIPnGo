#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

void clock_start(void);
uint32_t clock_get(void);
void clock_restart(void);

#endif
