#ifndef STATE_H
#define STATE_H

// State which is to be preserved between on/off cycles is in meter_state_t.
// Transient state is in transient_meter_state_t.

#include <stdbool.h>
#include <exposure.h>

#define STATE_BLOCK_LENGTH 128

// We leave four bytes of space at the beginning, in case we need
// to use this in future to specify the locations of other blocks.
#define STATE_BLOCK_START_ADDR ((void *)4)

typedef enum priority {
    NO_PRIORITY=0, SHUTTER_PRIORITY=1, APERTURE_PRIORITY=2
} priority_t;

typedef enum ui_mode {
    UI_MODE_READING=0,
    UI_MODE_MAIN_MENU,
    UI_MODE_CALIBRATE,
} ui_mode_t;

typedef union ui_mode_state {
    struct {
        uint8_t item_index;
        uint8_t voffset;
    } main_menu;
} ui_mode_state_t;

typedef enum meter_mode {
    METER_MODE_REFLECTIVE=0,
    METER_MODE_INCIDENT
} meter_mode_t;

// Full, half, quarter and eighth stops are calculated from
// the original values in 1/8EV stop increments. We get
// tenth stops using the additional bits in the
// STAGEx_LIGHT_VOLTAGE_TO_EV_TENTHS[] arrays. We fake 1/3
// stop readings using tenths: 0.2 -> 0.4 -> 1/3; 0.5-0.8 -> 2/3.
/*typedef enum precision_mode {
    PRECISION_MODE_FULL=1,
    PRECISION_MODE_HALF=2,
    PRECISION_MODE_THIRD=3,
    PRECISION_MODE_QUARTER=4,
    PRECISION_MODE_EIGHTH=8,
    PRECISION_MODE_TENTH=10
} precision_mode_t;*/
// gcc insisted on making this enum two bytes, I think because there
// was a prototype declaration in another file.
typedef uint_fast8_t precision_mode_t;
#define PRECISION_MODE_FULL 1
#define PRECISION_MODE_HALF 2
#define PRECISION_MODE_THIRD 3
#define PRECISION_MODE_QUARTER 4
#define PRECISION_MODE_EIGHTH 8
#define PRECISION_MODE_TENTH 10

typedef struct meter_state {
    uint8_t bcd_iso_digits[ISO_DECIMAL_MAX_DIGITS];
    uint8_t bcd_iso_length;
    ev_with_fracs_t stops_iso;

    priority_t priority;

    int8_t exp_comp;

    ui_mode_t ui_mode;
    ui_mode_state_t ui_mode_state;

    meter_mode_t meter_mode;
    precision_mode_t precision_mode;

    ev_with_fracs_t priority_aperture;
    ev_with_fracs_t priority_shutter_speed;
} meter_state_t;

#ifdef __AVR__
struct state_h_enum_size_test_struct {
    int test1[1-sizeof(ui_mode_t)];
    int test2[1-sizeof(meter_mode_t)];
    int test3[1-sizeof(precision_mode_t)];
};
#endif

// This is just to check that sizeof(meter_state) <= STATE_BLOCK_LENGTH.
// Attempting to specify an array of negative length will raise
// a compiler error.
struct meter_state_dummy_length_test_struct {
    int foo[STATE_BLOCK_LENGTH-sizeof(meter_state_t)];
};

extern meter_state_t global_meter_state;

void write_meter_state(const meter_state_t *ms);
void read_meter_state(meter_state_t *ms);
void initialize_global_meter_states();

typedef struct transient_meter_state {
    ev_with_fracs_t last_ev_with_fracs;
    ev_with_fracs_t aperture;
    ev_with_fracs_t shutter_speed;

    bool exposure_ready;

    uint8_t op_amp_resistor_stage;
} transient_meter_state_t;

extern transient_meter_state_t global_transient_meter_state;

#endif
