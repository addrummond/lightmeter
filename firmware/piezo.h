#ifndef PIEZO_H
#define PIEZO_H

#include <stdint.h>

void piezo_mic_init(void);
uint16_t piezo_mic_get_reading(void);

void piezo_out_init(void);
void piezo_set_period(unsigned channels, uint16_t period);
void piezo_turn_on(unsigned channels);
void piezo_pause(unsigned channels);
void piezo_unpause(unsigned channels);
void piezo_out_deinit(void);

#endif
