#ifndef SYSTICK_H
#define SYSTICK_H

#define TICKS_PER_SEC 8000000

#define SYSTICK 0xE000E010

#define STCTRL ((*(volatile uint32_t *)(SYSTICK + 0x00)))
#define STRELOAD ((*(volatile uint32_t *)(SYSTICK + 0x04)))
#define STCURRENT ((*(volatile uint32_t *)(SYSTICK + 0x08)))

#endif
