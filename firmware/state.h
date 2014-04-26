#ifndef STATE_H
#define STATE_H

// State which is to be preserved between on/off cycles.

#include <bcd.h>
#include <exposure.h>

#define STATE_BLOCK_LENGTH 128

// We leave four bytes of space at the beginning, in case we need
// to use this in future to specify the locations of other blocks.
#define STATE_BLOCK_START_ADDR ((void *)4)

typedef enum priority {
    NO_PRIORITY=0, SHUTTER_PRIORITY=1, APERTURE_PRIORITY=2
} priority_t;

typedef enum ui_mode {
    UI_MODE_DEFAULT=0,
    UI_MODE_MAIN_MENU,
    
    UI_MODE_CALIBRATE,
} ui_mode_t;

typedef enum meter_mode {
    METER_MODE_REFLECTIVE=0,
    METER_MODE_INCIDENT
} meter_mode_t;

typedef struct meter_state {
    uint8_t bcd_iso_digits[ISO_DECIMAL_MAX_DIGITS];
    uint8_t bcd_iso_length;
    uint8_t stops_iso;

    priority_t priority;

    int8_t exp_comp;

    uint8_t op_amp_resistor_stage;

    ui_mode_t ui_mode;

    meter_mode_t meter_mode;
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

typedef struct transient_meter_state {
    uint8_t aperture;
    uint8_t shutter_speed;
    aperture_string_output_t aperture_string;
    shutter_string_output_t shutter_speed_string;
} transient_meter_state_t;

extern transient_meter_state_t global_transient_meter_state;

// Check that enums are one byte (choosing an enum type at random).
// Emums should be one byte if gcc's -fshort-enums is enabled.
struct dummy {
    priority_t foo[1-sizeof(priority_t)];
};

#endif
