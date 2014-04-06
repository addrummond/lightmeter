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


typedef struct shutter_string_output {
    uint8_t chars[9];
    uint8_t length; // Does not include null terminator.
} shutter_string_output_t;
#define SHUTTER_STRING_OUTPUT_STRING(eso) ((eso).chars)

typedef struct aperture_string_output {
    uint8_t chars[11];
    uint8_t length; // Does not include null terminator.
} aperture_string_output_t;
#define APERTURE_STRING_OUTPUT_STRING(aso) ((aso).chars)

void shutter_speed_to_string(uint8_t speed, shutter_string_output_t *eso);
void aperture_to_string(uint8_t aperture, aperture_string_output_t *aso);
uint8_t x_given_shutter_speed_iso_ev(uint8_t given_x_, uint8_t iso_, uint8_t ev_, uint8_t x);
#define aperture_given_shutter_speed_iso_ev(a,b,c) x_given_shutter_speed_iso_ev(a,b,c,0)
#define shutter_speed_given_aperture_iso_ev(a,b,c) x_given_shutter_speed_iso_ev(a,b,c,1)
uint8_t iso_bcd_to_stops(uint8_t *digits, uint8_t length);

#endif
