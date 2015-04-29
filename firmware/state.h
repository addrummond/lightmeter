#ifndef STATE_H
#define STATE_H

// State which is to be preserved between on/off cycles is in meter_state_t.
// Transient state is in transient_meter_state_t.

#include <stdint.h>
#include <stdbool.h>
#include <exposure.h>

typedef enum fixing {
    FIXING_ISO = 0,
    FIXING_APERTURE,
    FIXING_SHUTTER_SPEED
} fixing_t;

typedef enum priority {
    NO_PRIORITY=0, SHUTTER_PRIORITY=1, APERTURE_PRIORITY=2
} priority_t;

typedef enum ui_mode {
    UI_MODE_INIT=0,
    UI_MODE_READING,
    UI_MODE_MAIN_MENU,
    UI_MODE_CALIBRATE,
} ui_mode_t;

typedef union ui_mode_state {
    struct {
        uint8_t start_line;
        int8_t current_accel_y;
        uint32_t ticks_waited;
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
typedef enum precision_mode {
    PRECISION_MODE_FULL=1,
    PRECISION_MODE_HALF=2,
    PRECISION_MODE_THIRD=3,
    PRECISION_MODE_QUARTER=4,
    PRECISION_MODE_EIGHTH=8,
    PRECISION_MODE_TENTH=10
} precision_mode_t;

typedef struct meter_state {
    uint8_t bcd_iso_digits[ISO_DECIMAL_MAX_DIGITS];
    uint8_t bcd_iso_length;
    uint8_t iso; // In 1/3 stops.

    fixing_t fixing;

    ev_with_fracs_t exp_comp;
    int8_t exp_comp_sign; // -1 or 1

    ui_mode_t ui_mode;
    ui_mode_state_t ui_mode_state;

    meter_mode_t meter_mode;
    precision_mode_t precision_mode;

    ev_with_fracs_t fixed_aperture;
    ev_with_fracs_t fixed_shutter_speed;
    uint8_t fixed_iso; // In 1/3 stops
} meter_state_t;

extern meter_state_t global_meter_state;

void write_meter_state(const meter_state_t *ms);
void read_meter_state(meter_state_t *ms);
void initialize_global_meter_state();
void initialize_global_transient_meter_state();

typedef struct transient_meter_state {
    ev_with_fracs_t last_ev_with_fracs;
    ev_with_fracs_t aperture;
    ev_with_fracs_t shutter_speed;
    uint8_t iso; // In 1/3 stops.

    bool exposure_ready;
} transient_meter_state_t;

extern transient_meter_state_t global_transient_meter_state;

#endif
