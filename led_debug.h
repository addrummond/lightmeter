#ifndef LED_DEBUG_H
#define LED_DEBUG_H

#include <stdint.h>

#define LED_LEFT_PB   0
#define LED_MIDDLE_PB 1
#define LED_RIGHT_PB  2

void debug_led_show_byte(uint8_t byte);

#endif
