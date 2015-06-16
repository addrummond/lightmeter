#ifndef METER_H
#define METER_H

#include <stdint.h>
#include <state.h>
#include <exposure.h>

typedef enum noise_filter_mode {
    NOISE_FILTER_MODE_NONE=0,
    NOISE_FILTER_MODE_MAINS
} noise_filter_mode_t;

void meter_init();
void meter_deinit();
void meter_set_mode(meter_mode_t mode);
uint32_t meter_take_raw_nonintegrated_reading();
void meter_take_raw_integrated_readings(uint16_t *outputs);
void meter_take_averaged_raw_readings_(uint16_t *outputs, unsigned n, noise_filter_mode_t nfm, int mode);
#define meter_take_averaged_raw_integrated_readings(x, y, z) meter_take_averaged_raw_readings_((x), (y), (z), 0)
#define meter_take_averaged_raw_nonintegrated_readings(x, y, z) meter_take_averaged_raw_readings_((x), (y), (z), 1)
ev_with_fracs_t meter_take_integrated_reading();

#endif
