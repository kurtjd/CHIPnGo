#include <stdint.h>
#include "systick.h"
#include "delay.h"
#include "sysclk.h"

#define MAX_TICKS 0x00FFFFFF

void delay(int ms) {
	long ticks = (ms * (AHB_CLOCK_SPEED / 1000)) - 1;

	/* The RELOAD register has a max value it can hold
	 * If the wanted delay is greater than this max,
	 * have to repeat the delay multiple times */
	while (ticks > 0) {
		STRELOAD |= (ticks > MAX_TICKS ? MAX_TICKS : ticks);
		STCURRENT = 0;
		STCTRL |= 0x05;
		
		while (!(STCTRL & 0x10000));

		ticks -= MAX_TICKS;
	}
}
