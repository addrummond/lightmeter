#ifndef METER_H
#define METER_H

#include <stdint.h>
#include <state.h>

void meter_init();
void meter_deinit();
void meter_set_mode(meter_mode_t mode);
uint32_t meter_take_raw_nonintegrated_reading();
uint32_t meter_take_raw_integrated_reading(uint32_t cycles);

#endif
