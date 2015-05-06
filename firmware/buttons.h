#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

void buttons_setup();
unsigned buttons_get_mask();
void buttons_clear_mask();
uint32_t buttons_get_ticks_pressed_for();

#endif
