#ifndef EXPOSURE_H
#define EXPOSURE_H

#include <stdint.h>

#define ISO_DECIMAL_MAX_DIGITS         7
#define AP_MIN_WHOLE_STOPS             0
#define AP_MAX_WHOLE_STOPS             10
#define SHUTTER_SPEED_MIN_WHOLE_STOPS  0
#define SHUTTER_SPEED_MAX_WHOLE_STOPS  20
#define ISO_MIN_WHOLE_STOPS            0
#define ISO_MAX_WHOLE_STOPS            48

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

#define EV_WITH_FRACS_TH 120
typedef int_fast16_t ev_with_fracs_t;

#define ev_with_fracs_init_from_120ths(evwf, v120) ((evwf) = (ev_with_fracs_t)v120)

#define ev_with_fracs_get_lowest_xths(evwf, x) \
    (((evwf)%EV_WITH_FRACS_TH)/(EV_WITH_FRACS_TH/(x)))

#define ev_with_fracs_get_nearest_xths(evwf, x) \
    ev_with_fracs_get_nearest_xths_(((evwf) % EV_WITH_FRACS_TH), (x))
#define ev_with_fracs_get_nearest_xths_(rem, x) \
    ev_with_fracs_get_nearest_xths__(rem, (rem / ((EV_WITH_FRACS_TH/x))), x)
#define ev_with_fracs_get_nearest_xths__(rem, n, x) \
    ev_with_fracs_get_nearest_xths___(rem, n*(EV_WITH_FRACS_TH/x), (n+1)*(EV_WITH_FRACS_TH/x), n, (n+1))
#define ev_with_fracs_get_nearest_xths___(rem, low, high, nlow, nhigh) \
    ev_with_fracs_get_nearest_xths____((rem-low), (high-rem), nlow, nhigh)
#define ev_with_fracs_get_nearest_xths___(rem, low, high, nlow, nhigh) \
    ev_with_fracs_get_nearest_xths____((rem-low), (high-rem), nlow, nhigh)
#define ev_with_fracs_get_nearest_xths____(lowdiff, highdiff, nlow, nhigh) \
    (highdiff > lowdiff ? nlow : nhigh)

#define ev_with_fracs_init(evwf)                   ((evwf) = 0)
#define ev_with_fracs_init_from_wholes(evwf, t)    ((evwf) = (t)*EV_WITH_FRACS_TH)
#define ev_with_fracs_init_from_quarters(evwf, t)  ((evwf) = (t)*(EV_WITH_FRACS_TH/4))
#define ev_with_fracs_init_from_halves(evwf, t)    ((evwf) = (t)*(EV_WITH_FRACS_TH/2))
#define ev_with_fracs_init_from_eighths(evwf, t)   ((evwf) = (t)*(EV_WITH_FRACS_TH/8))
#define ev_with_fracs_init_from_thirds(evwf, t)    ((evwf) = (t)*(EV_WITH_FRACS_TH/3))
#define ev_with_fracs_init_from_tenths(evwf, t)    ((evwf) = (t)*(EV_WITH_FRACS_TH/10))
#define ev_with_fracs_get_wholes(evwf)             ((evwf) / EV_WITH_FRACS_TH)
#define ev_with_fracs_get_lowest_thirds(evwf)      ev_with_fracs_get_lowest_xths(evwf, 3)
#define ev_with_fracs_get_nearest_thirds(evwf)     ev_with_fracs_get_nearest_xths(evwf, 3)
#define ev_with_fracs_set_thirds(evwf, v)          ((evwf) = ((evwf)/EV_WITH_FRACS_TH) + ((v)*(EV_WITH_FRACS_TH/3)))
#define ev_with_fracs_get_lowest_eighths(evwf)     ev_with_fracs_get_lowest_xths(evwf, 8)
#define ev_with_fracs_get_nearest_eighths(evwf)    ev_with_fracs_get_nearest_xths(evwf, 8)
#define ev_with_fracs_get_lowest_tenths(evwf)      ev_with_fracs_get_lowest_xths(evwf, 10)
#define ev_with_fracs_get_nearest_tenths(evwf)     ev_with_fracs_get_nearest_xths(evwf, 10)
#define ev_with_fracs_set_tenths(evwf, v)          ((evwf) = ((evwf)/EV_WITH_FRACS_TH) + ((v)*(EV_WITH_FRACS_TH/10)))
#define ev_with_fracs_is_whole(evwf)               ((evwf) % EV_WITH_FRACS_TH == 0)
#define ev_with_fracs_zero_fracs(evwf)             ((evwf) -= ((evwf)%EV_WITH_FRACS_TH))

enum precision_mode;
void shutter_speed_to_string(ev_with_fracs_t shutter_speed, shutter_string_output_t *eso, enum precision_mode precision_mode);
void aperture_to_string(ev_with_fracs_t aperture, aperture_string_output_t *aso, enum precision_mode precision_mode);
ev_with_fracs_t z_given_x_y_ev(ev_with_fracs_t given_x, ev_with_fracs_t given_y, ev_with_fracs_t evwf, uint_fast8_t x);
#define aperture_given_shutter_speed_iso_ev(a,b,c) z_given_x_y_ev((a),(b),(c),0)
#define shutter_speed_given_aperture_iso_ev(a,b,c) z_given_x_y_ev((a),(b),(c),1)
#define iso_given_aperture_shutter_speed_ev(a,b,c) z_given_x_y_ev((a),(b),(c),2)

ev_with_fracs_t average_ev_with_fracs(const ev_with_fracs_t *evwfs, unsigned length);

unsigned iso_in_third_stops_to_bcd(uint_fast8_t iso, uint8_t *digits);
uint_fast8_t iso_bcd_to_third_stops(uint8_t *digits, unsigned length);

ev_with_fracs_t get_ev100_at_voltage(uint_fast8_t voltage, uint_fast8_t op_amp_resistor_stage);
uint_fast8_t convert_from_reference_voltage(uint_fast16_t adc_out);

unsigned ev_at_100_to_bcd_lux(ev_with_fracs_t evwf, uint8_t *digits);
#define EV_AT_100_TO_BCD_LUX_RESULT_PRECISION 2
#define EV_AT_100_TO_BCD_LUX_BCD_LENGTH       (7+EV_AT_100_TO_BCD_LUX_RESULT_PRECISION)

// Note that these give the eighth/third immediately <= the nearest tenth
// (i.e. they don't round up).
#define eighth_below_tenth(e)  ((e) == 0 ? 0 : ((e) <= 5 ? (e)-1 : (e)-2))
#define third_below_tenth(e)  ((e) <= 3 ? 0 : ((e) <= 6 ? 1 : 2))

#define thirds_from_tenths(e)  ((e) > 6 ? 2 : ((e) > 2 ? 1 : 0))
#define thirds_from_eighths(e) ((e) > 4 ? 2 : ((e) > 1 ? 1 : 0))
#define tenths_from_thirds(e)  ((e) == 0 ? 0 : ((e) == 1 ? 3 : 7))
#define eighths_from_thirds(e) ((e) == 2 ? 5 : ((e) == 1 ? 3 : 0))

// Integer divide with rounding.
#define round_divide(n, by) (((n)/(by)) + (((n) % (by) > ((by)+1)/2) ? 1 : 0))

#endif
