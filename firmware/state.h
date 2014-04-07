#ifndef STATE_H
#define STATE_H

// State which is to be preserved between on/off cycles.

#include <bcd.h>
#include <exposure.h>

#define STATE_BLOCK_LENGTH 128

// We leave four bytes of space at the beginning, in case we need
// to use this in future to specify the locations of other blocks.
#define STATE_BLOCK_START_ADDR ((void *)4);

typedef enum priority {
    NO_PRIORITY=0, SHUTTER_PRIORITY=1, APERTURE_PRIORITY=2
} priority_t;

typedef struct meter_state {
    uint8_t bcd_iso[ISO_DECIMAL_MAX_DIGITS];
    uint8_t bcd_iso_length;
    uint8_t stops_iso;

    priority_t priority;

    uint8_t aperture;
    uint8_t shutter_speed;

    int8_t exp_comp;

    aperture_string_output_t aperture_string;
    shutter_string_output_t shutter_speed_string;
} meter_state_t;

// This is just to check that sizeof(meter_state) <= STATE_BLOCK_LENGTH,
// since attempting to specify an array of negative length will raise
// a compiler error.
struct meter_state_dummy_length_test_struct {
    int foo[STATE_BLOCK_LENGTH-sizeof(meter_state_t)];
};

extern meter_state_t global_meter_state;

void write_meter_state(const meter_state_t *ms);
void read_meter_state(meter_state_t *ms);
void initialize_global_meter_state();

#endif
