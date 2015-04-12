#ifndef METER_H
#define METER_H

#include <stdint.h>
#include <state.h>

#define METER_NUMBER_OF_INTEGRATING_STAGES 4

void meter_init();
void meter_deinit();
void meter_set_mode(meter_mode_t mode);
uint32_t meter_take_raw_nonintegrated_reading();
void meter_take_raw_integrated_readings(uint16_t *outputs);
void meter_take_averaged_raw_integrated_readings(uint16_t *outputs, unsigned n);

#endif
