#ifndef PIEZO_H
#define PIEZO_H

#include <stdint.h>

void piezo_init(void);
void piezo_set_period(unsigned channels, uint16_t period);
void piezo_turn_on(unsigned channels);
void piezo_pause(unsigned channels);
void piezo_deinit(void);

#endif
