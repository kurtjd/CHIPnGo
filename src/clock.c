// TODO: Use timer instead of systick

#include <stdint.h>
#include "systick.h"
#include "clock.h"
#include "sysclk.h"

static volatile uint32_t clock = 0;

void SysTick_Handler(void) {
	clock++;
}

void clock_start(void) {
	STRELOAD = (AHB_CLOCK_SPEED / 1000) - 1;
	STCURRENT = 0;
	STCTRL = 7;
}

uint32_t clock_get(void) {
	return clock;
}

void clock_restart(void) {
	clock = 0;
}
