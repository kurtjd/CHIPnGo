#ifndef SYSCLK_H
#define SYSCLK_H

extern long CLOCK_SPEED;
extern long APB1_CLOCK_SPEED;
extern long APB2_CLOCK_SPEED;
extern long ADC_CLOCK_SPEED;
extern long AHB_CLOCK_SPEED;

void set_sysclk(int mhz);

#endif
