#include "buttons.h"

#include "clock.h"
#include "gpio.h"
#include "led.h"

#include "display.h"

#define NVIC 0xE000E100
#define NVIC_ISER0 (*((volatile uint32_t *)(NVIC + 0x00)))
#define NVIC_ISER1 (*((volatile uint32_t *)(NVIC + 0x04)))

#define EXTI 0x40010400
#define EXTI_IMR (*((volatile uint32_t *)(EXTI + 0x00)))
#define EXTI_RTSR (*((volatile uint32_t *)(EXTI + 0x08)))
#define EXTI_FTSR (*((volatile uint32_t *)(EXTI + 0x0C)))
#define EXTI_PR (*((volatile uint32_t *)(EXTI + 0x14)))

#define AFIO 0x40010000
#define AFIO2 (*((volatile uint32_t *)(AFIO + 0x0C)))
#define AFIO3 (*((volatile uint32_t *)(AFIO + 0x10)))

#define BTN_LEFT_INT_NVIC (1 << 10)
#define BTN_OTHERS_INT_NVIC (1 << 23)
#define BTN_A_B (1 << 8)

#define BTN_LEFT_INT_EXTI (1 << 4)
#define BTN_RIGHT_INT_EXTI (1 << 7)
#define BTN_UP_INT_EXTI (1 << 6)
#define BTN_DOWN_INT_EXTI (1 << 5)

#define BTN_A_INT_EXTI (1 << 11)
#define BTN_B_INT_EXTI (1 << 10)

#define BTN_LEFT_MODE_PU (1 << 19)
#define BTN_UP_MODE_PU (1 << 23)
#define BTN_DOWN_MODE_PU (1 << 27)
#define BTN_RIGHT_MODE_PU (1 << 31)

#define BTN_A_MODE_PU (1 << 15)
#define BTN_B_MODE_PU (1 << 11)

#define BTN_LEFT_MODE_FI (1 << 18)
#define BTN_UP_MODE_FI (1 << 22)
#define BTN_DOWN_MODE_FI (1 << 26)
#define BTN_RIGHT_MODE_FI (1 << 30)

#define BTN_A_MODE_FI (1 << 14)
#define BTN_B_MODE_FI (1 << 10)

#define BOUNCE_WAIT 10  // Spec says 5ms max but really need twice that

static int btn_status[NUM_BUTTONS] = {0};
static uint32_t last_press = 0;

static int _get_btn_idx(enum Button btn) {
    switch (btn) {
    case BTN_LEFT:
        return 0;
    case BTN_RIGHT:
        return 1;
    case BTN_UP:
        return 2;
    case BTN_DOWN:
        return 3;
    case BTN_A:
        return 4;
    case BTN_B:
        return 5;
    }

    return 0;
}

static bool _btn_pressed_raw(enum Button btn) {
    return !(GPIOB_IDR & (1 << btn));  // Active-low logic
}

static void _update_status(enum Button btn) {
    int idx = _get_btn_idx(btn);

    if (_btn_pressed_raw(btn))
        btn_status[idx] = 1;
    else if (btn_status[idx] == 1)
        btn_status[idx] = 2;
}

static void _btn_interrupt(void) {
    // Wait for bounce to settle
    if ((clock_get() - last_press) > BOUNCE_WAIT) {
        // Find pin that generated interrupt
        int pin = 0;
        uint32_t pr = EXTI_PR;
        while (!(pr & 1)) {
            pr >>= 1;
            pin++;
        }

        EXTI_PR |= (1 << pin);
        _update_status(pin);  // Pin maps to a Button

        last_press = clock_get();
    }
}

void EXTI4_IRQHandler() {
    _btn_interrupt();
}

void EXTI9_5_IRQHandler() {
    _btn_interrupt();
}

void EXTI15_10_IRQHandler() {
    _btn_interrupt();
}

void buttons_init(void) {
    // Clear floating input bits (which are set to 1 at reset)
    GPIOB_CRL &= ~(BTN_LEFT_MODE_FI | BTN_UP_MODE_FI);
    GPIOB_CRL &= ~(BTN_DOWN_MODE_FI | BTN_RIGHT_MODE_FI);
    GPIOB_CRH &= ~(BTN_A_MODE_FI | BTN_B_MODE_FI);
    
    display_print(0, 0, "1");

    // Set inputs as internal pull-up
    GPIOB_CRL |= (BTN_LEFT_MODE_PU | BTN_UP_MODE_PU);
    GPIOB_CRL |= (BTN_DOWN_MODE_PU | BTN_RIGHT_MODE_PU);
    GPIOB_CRH |= (BTN_A_MODE_PU | BTN_B_MODE_PU);

    display_print(0, 1, "2");

    // Activate pull-up resistors
    GPIOB_ODR |= ((1 << 4) | (1 << 5) | (1 << 6) | (1 << 7));
    GPIOB_ODR |= ((1 << 10) | (1 << 11));

    display_print(0, 2, "3");

    // AFSIO EXTI set to port B
    RCC_APB2ENR |= 1;

    display_print(0, 3, "4");

    // Directional
    AFIO2 |= ((1) | (1 << 4) | (1 << 8) | (1 << 12));

    // A/B
    AFIO3 |= ((1 << 8) | (1 << 12));

    display_print(0, 4, "5");

    // Unmask external interrupts
    EXTI_IMR |= (BTN_LEFT_INT_EXTI | BTN_RIGHT_INT_EXTI);
    EXTI_IMR |= (BTN_UP_INT_EXTI | BTN_DOWN_INT_EXTI);
    EXTI_IMR |= (BTN_A_INT_EXTI | BTN_B_INT_EXTI);

    display_print(0, 5, "6");

    // Trigger on rising AND falling edge
    EXTI_RTSR |= (BTN_LEFT_INT_EXTI | BTN_RIGHT_INT_EXTI);
    EXTI_RTSR |= (BTN_UP_INT_EXTI | BTN_DOWN_INT_EXTI);
    EXTI_RTSR |= (BTN_A_INT_EXTI | BTN_B_INT_EXTI);
    EXTI_FTSR |= (BTN_LEFT_INT_EXTI | BTN_RIGHT_INT_EXTI);
    EXTI_FTSR |= (BTN_UP_INT_EXTI | BTN_DOWN_INT_EXTI);
    EXTI_FTSR |= (BTN_A_INT_EXTI | BTN_B_INT_EXTI);

    // Finally enable interrupts on the NVIC
    NVIC_ISER0 |= (BTN_LEFT_INT_NVIC | BTN_OTHERS_INT_NVIC);
    NVIC_ISER1 |= (BTN_A_B);

    display_print(0, 6, "7");
}

bool btn_pressed(enum Button btn) {
    return (btn_status[_get_btn_idx(btn)] == 1);
}

bool btn_released(enum Button btn) {
    int idx = _get_btn_idx(btn);
    if (btn_status[idx] == 2) {
        btn_status[idx] = 0;
        return true;
    }

    return false;
}
