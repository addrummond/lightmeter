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

#define SHUTTER_SPEED_MIN 0
#define SHUTTER_SPEED_MAX (20*8)

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
    uint8_t chars[8];
    uint8_t length; // Does not include null terminator.
} aperture_string_output_t;
#define APERTURE_STRING_OUTPUT_STRING(aso) ((aso).chars)

typedef struct ev_with_fracs {
    uint8_t ev; // The ev value in 1/8 EV steps starting from -5EV.
    uint8_t fracs;
    // Low nibble of fracs + (ev/8)*3 gives EV in 1/3 stops starting from -5EV.
    // High nibble of fracs + (ev/8)*10 gives EV in 1/10 stops starting from -5EV.

    // Mini essay: Could we ever get 10/10 or 3/3?
    //
    // One might think that 10/10 could occur in the following
    // situation. Say that the ISO is 100+9/10 and the EV@ISO100 is x+1/10 EV,
    // and that in eighths, the ISO is 100+7/8 and the EV@ISO100 is x+0/8. Thus,
    // in eighths the EV for the specified ISO is x+7/8, whereas in tenths it
    // is x+1 (i.e. x+10/10).
    //
    // Fortunately, in this case, 1/10 is closer to 1/8 than it is to 0/8. Thus,
    // this situation should never arise if the exposure tables are rounded correctly.
    // One would need to find a value v such that diff(v,1/10) < diff(v,0/10)
    // and yet diff(v, 0/8) < diff(v, 1/8). No such value can exist.
    // Similarly for thirds.
    //
    // 1/10, 9/10 -- 1/8, 7/8 -- OK
    // 2/10, 8/10 -- 2/8, 6/8 -- OK
    // 3/10, 7/10 -- 2/8, 6/8 -- OK
    // 4/10, 6/10 -- 3/8, 5/8 -- OK
    //
    // 1/3, 2/3   --          -- OK
    //
    // We can therefore trust
    // that calculations based on eighths (if they involve only addition and
    // subtraction) will always give the same whole component for the EV value
    // as calculations in tenths and thirds. This means we don't have to deal
    // with correctly displaying values like x+10/10 x+3/3, which is nice.
} ev_with_fracs_t;

#define ev_with_fracs_get_thirds(evwf)    ((evwf).fracs & 0x0F)
#define ev_with_fracs_set_thirds(evwf, v) ((evwf).fracs |= (v) & 0b111)
#define ev_with_fracs_get_tenths(evwf)    ((evwf).fracs >> 4)
#define ev_with_fracs_set_tenths(evwf, v) ((evwf).fracs |= (v) << 4)
#define ev_with_fracs_is_whole(evwf)      ((evwf).fracs == 0)
#define ev_with_fracs_make_whole(evwf)    ((evwf).fracs = 0)

void shutter_speed_to_string(ev_with_fracs_t shutter_speed, shutter_string_output_t *eso, uint8_t precision_mode);
void aperture_to_string(ev_with_fracs_t aperture, aperture_string_output_t *aso, uint8_t precision_mode);
ev_with_fracs_t x_given_y_iso_ev(uint8_t given_x_, uint8_t iso_, ev_with_fracs_t ev, uint8_t x);
#define aperture_given_shutter_speed_iso_ev(a,b,c) x_given_y_iso_ev(a,b,c,0)
#define shutter_speed_given_aperture_iso_ev(a,b,c) x_given_y_iso_ev(a,b,c,1)
uint8_t iso_bcd_to_third_stops(uint8_t *digits, uint8_t length);

ev_with_fracs_t get_ev100_at_temperature_voltage(uint8_t temperature, uint8_t voltage, uint8_t op_amp_resistor_stage);
uint8_t convert_from_reference_voltage(uint16_t adc_out);

// Note that these gives the tenth/third immediately <= the nearest eighth
// (i.e. they don't round up).
#define tenth_below_eighth(e) (((e)&0b111) > 3 ? ((e)&0b111) + 1 : ((e)&0b111))
#define third_below_eighth(e) ((e) <= 2 ? 0 : ((e) <= 5 ? 1 : 2))

#define thirds_from_tenths(e)  ((e) > 6 ? 2 : ((e) > 2 ? 1 : 0))
#define tenths_from_thirds(e)  ((e) == 0 ? 0 : ((e) == 1 ? 3 : 7))
#define eighths_from_thirds(e) ((e) < 2 ? 0 : ((e) < 5 ? 1 : 2))

#endif
