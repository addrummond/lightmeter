#ifndef EXPOSURE_H
#define EXPOSURE_H

#define SS_1M       0
#define SS_8S       24
#define SS_1S       48
#define SS_8TH      64
#define SS_15TH     80
#define SS_1000TH   128
#define SS_8000TH   152
#define SS_10000TH  154
#define SS_16000TH  160

#define SS_MIN      0
#define SS_MAX      160

#define AP_F8       48
#define AP_F9_5     52

#define AP_MIN      0
#define AP_MAX      80

#define ISO_MIN     0
#define ISO_MAX     144 // 1600000
#define ISO_DECIMAL_MAX_DIGITS 7

#define EV_MIN      0
#define EV_MAX      254

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

typedef struct shutter_string_output {
    uint8_t chars[9];
    uint8_t length; // Does not include null terminator.
} shutter_string_output_t;
#define SHUTTER_STRING_OUTPUT_STRING(eso) ((eso).chars)

typedef struct aperture_string_output {
    uint8_t chars[8];
    uint8_t length; // Does not include null terminator.
} aperture_string_output_t;
#define APERTURE_STRING_OUTPUT_STRING(aso) ((aso).chars)

typedef struct ev_with_tenths {
    uint8_t ev; // The ev value in 1/8 EV steps starting from -5EV.
    uint8_t tenths; // This + (ev/8)*10 gives EV in 1/10 stops starting from -5EV.
} ev_with_tenths_t;

enum precision_mode;
void shutter_speed_to_string(uint8_t speed, shutter_string_output_t *eso);
void aperture_to_string(ev_with_tenths_t apev, aperture_string_output_t *aso, precision_mode_t precision_mode);
ev_with_tenths_t x_given_y_iso_ev(uint8_t given_x_, uint8_t iso_, ev_with_tenths_t ev, uint8_t x);
#define aperture_given_shutter_speed_iso_ev(a,b,c) x_given_y_iso_ev(a,b,c,0)
#define shutter_speed_given_aperture_iso_ev(a,b,c) x_given_y_iso_ev(a,b,c,1)
uint8_t iso_bcd_to_stops(uint8_t *digits, uint8_t length);

uint8_t count_bits_in_word(uint16_t word);
ev_with_tenths_t get_ev100_at_temperature_voltage(uint8_t temperature, uint8_t voltage, uint8_t op_amp_resistor_stage);
uint8_t convert_from_reference_voltage(uint16_t adc_out);

#endif
