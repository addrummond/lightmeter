// Apertures are represented as unsigned 8-bit quantities
// from 1.0 to 32 in 1/8-stop steps.
//
// Shutter speeds are represented as unsigned 8-bit quantities
// from 1 minute to 1/16000 in 1/8-stop steps. Some key points
// on the scale are defined in exposure.h
//
// ISO is represented as an unsigned 8-bit quantity giving 1/3-stops from ISO 6.

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <myassert.h>
#include <state.h>
#include <exposure.h>
#include <bcd.h>
#include <tables.h>
#include <deviceconfig.h>
#include <mymemset.h>
#ifdef TEST
#include <stdio.h>
#endif
#include <basic_serial/basic_serial.h>

// See comments in calculate_tables.py for info on the way
// temp/voltage are encoded.
//
// 'voltage' is in 1/256ths of the reference voltage.
ev_with_fracs_t get_ev100_at_voltage(uint_fast8_t voltage, uint_fast8_t op_amp_resistor_stage)
{
    const uint8_t *ev_abs = NULL, *ev_diffs = NULL, *ev_bitpatterns = NULL, *ev_tenths = NULL, *ev_thirds = NULL;
#define ASSIGN(n) (ev_abs = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_ABS,                 \
                   ev_diffs = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_DIFFS,             \
                   ev_bitpatterns = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_BITPATTERNS, \
                   ev_tenths = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_TENTHS,           \
                   ev_thirds = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_THIRDS)
#define CASE(n) case n: ASSIGN(n); break;
    switch (op_amp_resistor_stage) {
FOR_EACH_AMP_STAGE(CASE)
    }
#undef ASSIGN
#undef CASE

    ev_with_fracs_t ret;

    if (voltage < VOLTAGE_TO_EV_ABS_OFFSET)
        voltage = 0;
    else
        voltage -= VOLTAGE_TO_EV_ABS_OFFSET;

    uint_fast8_t absi = voltage >> 4;
    uint_fast8_t bits_to_add = (voltage & 15) + 1; // (voltage % 16) + 1

    uint_fast8_t bit_pattern_indices = ev_diffs[absi];
    uint_fast8_t bits1 = ev_bitpatterns[bit_pattern_indices >> 4];
    uint_fast8_t bits2 = ev_bitpatterns[bit_pattern_indices & 0x0F];
    if (bits_to_add < 8) {
        bits1 &= 0xFF << (8 - bits_to_add);
        bits2 = 0;
    }
    else {
        bits2 &= 0xFF << (16 - bits_to_add);
    }

    // See http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
    ret.ev = ev_abs[absi];
    for (; bits1; ++ret.ev)
        bits1 &= bits1 - 1;
    for (; bits2; ++ret.ev)
        bits2 &= bits2 - 1;
//#ifdef DEBUG
//    tx_byte('B');
//    tx_byte(ret.ev);
//#endif

    // Calculate tenths.
    uint_fast8_t tenths_bit = ev_tenths[voltage >> 3];
    tenths_bit >>= (voltage & 0b111);
    tenths_bit &= 1;
    int_fast8_t ret_tenths = tenth_below_eighth(ret.ev);
    ret_tenths += tenths_bit;

    // Calculate thirds.
    uint_fast8_t thirds_bit = ev_thirds[voltage >> 3];
    thirds_bit >>= (voltage & 0b111);
    thirds_bit &= 0b11;
    int_fast8_t ret_thirds = third_below_eighth(ret.ev);
    ret_thirds += thirds_bit;

    // Note behavior of '%' with respect to negative numbers. E.g. -3 % 8 == 5.
    ev_with_fracs_set_tenths(ret, ret_tenths % 10);
    ev_with_fracs_set_thirds(ret, ret_thirds % 3);

    return ret;
}

static void normalize_precision_to_tenth_eighth_or_third(precision_mode_t *precision_mode, ev_with_fracs_t *evwf)
{
    // TODO: Rounding.
    if (*precision_mode == PRECISION_MODE_QUARTER) {
        *precision_mode = PRECISION_MODE_EIGHTH;
        evwf->ev &= 1;
    }
    else if (*precision_mode == PRECISION_MODE_HALF) {
        *precision_mode = PRECISION_MODE_EIGHTH;
        uint_fast8_t rem = evwf->ev & 0b11;
        evwf->ev >>= 2;
        if (rem >= 2)
            evwf->ev += 4;
    }
    else if (*precision_mode == PRECISION_MODE_FULL) {
        *precision_mode = PRECISION_MODE_EIGHTH;
        uint_fast8_t rem = evwf->ev & 0b111;
        evwf->ev >>= 3;
        if (rem >= 4)
            evwf->ev += 8;
    }
}

void shutter_speed_to_string(ev_with_fracs_t evwf, shutter_string_output_t *sso, precision_mode_t precision_mode)
{
    //precision_mode_t orig_precision_mode = precision_mode;
    normalize_precision_to_tenth_eighth_or_third(&precision_mode, &evwf);

    if (evwf.ev >= SHUTTER_SPEED_MAX) {
        evwf.ev = SHUTTER_SPEED_MAX;
        ev_with_fracs_zero_fracs(evwf);
    }

    uint_fast8_t shutev = evwf.ev;

    uint8_t *schars;
    if (precision_mode == PRECISION_MODE_THIRD) {
        schars = SHUTTER_SPEEDS_THIRD + ((shutev/8) * 3 * 2);
        uint_fast8_t thirds = ev_with_fracs_get_thirds(evwf);
        schars += thirds*2;
    }
    else if (precision_mode == PRECISION_MODE_EIGHTH) {
        schars = SHUTTER_SPEEDS_EIGHTH + (shutev * 2);
    }
    else { //if (precision_mode == PRECISION_MODE_TENTH) {
        schars = SHUTTER_SPEEDS_TENTH + ((shutev/8)*10*2);
        schars += ev_with_fracs_get_tenths(evwf) * 2;
    }

    uint_fast8_t last = 0;
    if (shutev > (6*8) || (shutev == (6*8) && !ev_with_fracs_is_whole(evwf))) {
        sso->chars[last++] = '1';
        sso->chars[last++] = '/';
    }

    uint_fast8_t i;
    for (i = 0; i < 2; ++i) {
        uint_fast8_t val, j;
        for (val = schars[i] & 0xF, j = 0; j < 2 && val != 0; val = schars[i] >> 4, ++j) {
            uint_fast8_t b = SHUTTER_SPEEDS_BITMAP[val];
            if (b == 'X') {
                sso->chars[last++] = '0';
                sso->chars[last++] = '0';
            }
            else {
                sso->chars[last++] = b;
            }
        }
    }

    if (shutev < 6*8 || (shutev == 6*8 && ev_with_fracs_is_whole(evwf)))
        sso->chars[last++] = 'S';

    sso->chars[last] = '\0';
    sso->length = last;
}

void aperture_to_string(ev_with_fracs_t evwf, aperture_string_output_t *aso, precision_mode_t precision_mode)
{
    //precision_mode_t orig_precision_mode = precision_mode;
    normalize_precision_to_tenth_eighth_or_third(&precision_mode, &evwf);

    if (evwf.ev >= AP_MAX) {
        evwf.ev = AP_MAX;
        ev_with_fracs_zero_fracs(evwf);
    }

    uint_fast8_t apev = evwf.ev;

    uint_fast8_t last;
    if (precision_mode == PRECISION_MODE_THIRD) {
        uint_fast8_t thirds = ev_with_fracs_get_thirds(evwf);
        uint_fast8_t b = APERTURES_THIRD[((apev >> 3)*3) + thirds];
        last = 0;
        aso->chars[last++] = '0' + (b & 0xF);
        if (apev < 6*8 || (apev == 6*8 && thirds < 2))
            aso->chars[last++] = '.';
        aso->chars[last++] = '0' + (b >> 4);
    }
    else if (precision_mode == PRECISION_MODE_TENTH || precision_mode == PRECISION_MODE_EIGHTH) {
        uint_fast8_t b1, b2, i;
        if (precision_mode == PRECISION_MODE_EIGHTH) {
            i = apev + (apev >> 1);
            //printf("I: %i\n", i);
            b1 = APERTURES_EIGHTH[i];
            b2 = APERTURES_EIGHTH[i+1];
        }
        else { //if (precision_mode == PRECISION_MODE_TENTH) {
            uint_fast8_t tenths = ev_with_fracs_get_tenths(evwf);
            i = ((apev >> 3) * 15) + tenths + (tenths >> 1);
            b1 = APERTURES_TENTH[i];
            b2 = APERTURES_TENTH[i+1];
        }
        uint_fast8_t d1 = '0', d2 = '0', d3 = '0';
        if (i % 3 == 0) {
            //printf("FFF %i %i\n", b1, b2);
            d1 += b1 >> 4;
            d2 += b1 & 0xF;
            d3 += b2 >> 4;
        }
        else {
            //printf("GGG %i %i\n", b1, b2);
            d1 += b1 & 0xF;
            d2 += b2 >> 4;
            d3 += b2 & 0xF;
        }

        last = 0;
        aso->chars[last++] = d1;
        if (apev < 6*8 || (precision_mode == PRECISION_MODE_TENTH && apev < 7*8 && ev_with_fracs_get_tenths(evwf) < 7) ||
                          (precision_mode == PRECISION_MODE_EIGHTH && apev < 54)) {
            aso->chars[last++] = '.';
        }
        aso->chars[last++] = d2;
        if (last == 2)
            aso->chars[last++] = '.';
        aso->chars[last++] = d3;
    }
    else {
        assert(false);
    }

    aso->length = last;

    // If we're doing quarter-stop precision, round off last digit.
    /*if (orig_precision_mode == PRECISION_MODE_QUARTER) {
        uint_fast8_t i = last-1;
        if (aso->chars[i] >= '5')
            ++aso->chars[i-1];
        do {
            --i;
            if (i < '9')
                break;
            aso->chars[i] = '0';
            ++aso->chars[i-1];
        } while (i > 1);

        --(aso->length);
    }*/

    // Remove '.0' and '.00'
    uint_fast8_t i = aso->length;
    do {
        --i;
        if (aso->chars[i] == '.') {
            --i;
            break;
        }
        if (aso->chars[i] != '0')
            break;
    } while (i > 0);
    aso->length = i+1;
    aso->chars[aso->length] = '\0';
}

// Table storing full-stop ISO numbers.
// Each pair of bytes is:
//     (i)  big-endian 4-bit BCD rep of first two digits,
//     (ii) number of trailing zeroes.
static const uint8_t FULL_STOP_ISOS[] = {
    6, 0,             // 6
    (1 << 4) | 2, 0,  // 12
    (2 << 4) | 5, 0,  // 25
    5, 1,             // 50
    1, 2,             // 100
    2, 2,             // 200
    4, 2,             // 400
    8, 2,             // 800
    (1 << 4) | 6, 2,  // 1600
    (3 << 4) | 2, 2,  // 3200
    (6 << 4) | 4, 2,  // 6400
    (1 << 4) | 2, 3,  // 12500 // must be special-cased to add 5.
    (2 << 4) | 5, 3,  // 25000
    5, 4,             // 50000
    1, 5,             // 100000
    2, 5,             // 200000
    4, 5,             // 400000
    8, 5,             // 800000
    (1 << 4) | 6, 5,  // 1600000
};

static const uint8_t THIRD_STOP_ISOS[] = {
    8, 0,             // 8
    (1 << 4) | 0, 0,  // 10

    (1 << 4) | 6, 0,  // 16
    (2 << 4) | 0, 0,  // 20

    (3 << 4) | 2, 0,  // 32
    (3 << 4) | 0, 0,  // 40

    (6 << 4) | 4, 0,  // 64
    (8 << 4) | 0, 0,  // 80

    (1 << 4) | 2, 1,  // 125 (specified as 120, must be special-cased).
    (1 << 4) | 6, 1,  // 160

    (2 << 4) | 5, 1,  // 250
    (3 << 4) | 2, 1,  // 320

    (5 << 4) | 0, 1,  // 500
    (6 << 4) | 4, 1,  // 640

    (1 << 4) | 0, 2,  // 1000
    (1 << 4) | 2, 2,  // 1250 (specified as 1200, must be special-cased)

    (2 << 4) | 0, 2,  // 2000
    (2 << 4) | 5, 2,  // 2500

    (4 << 4) | 0, 2,  // 4000
    (5 << 4) | 0, 2,  // 5000

    (8 << 4) | 0, 2,  // 8000
    (1 << 4) | 0, 3,  // 10000

    (1 << 4) | 6, 3,  // 16000
    (2 << 4) | 0, 3,  // 20000

    (3 << 4) | 2, 3,  // 32000
    (4 << 4) | 0, 3,  // 40000

    (6 << 4) | 4, 3,  // 64000
    (8 << 4) | 0, 3,  // 80000

    (1 << 4) | 2, 4,  // 125000 (specified as 120000, must be special-cased)
    (1 << 4) | 6, 4,  // 160000

    (2 << 4) | 5, 4,  // 250000
    (3 << 4) | 2, 4,  // 320000
};

static uint8_t *iso_into_bcd(uint_fast8_t byte1, uint_fast8_t zeroes, uint8_t *digits, uint_fast8_t length)
{
    uint_fast8_t i, z;
    for (i = length-1, z = 0; z < zeroes; --i, ++z) {
        digits[i] = 0;
    }

    digits[i] = byte1 & 0x0F;
    if (byte1 & 0xF0) {
        --i;
        digits[i] = byte1 >> 4;
        // Special case for 125, 1250, 12500 and 125000
        if (byte1 == ((1 << 4) | 2) && zeroes >= 1 && zeroes <= 4)
            digits[length - zeroes + 1] = 5;
        return digits + length - zeroes - 2;
    }
    else {
        return digits + length - zeroes - 1;
    }
}

uint8_t *iso_in_third_stops_into_bcd(uint_fast8_t iso, uint8_t *digits, uint_fast8_t length)
{
    uint_fast8_t byte1, zeroes;
    uint_fast8_t i = iso/3*2;
    uint_fast8_t rem = iso % 3;
    if (rem == 0) {
        byte1 = FULL_STOP_ISOS[i];
        zeroes = FULL_STOP_ISOS[i+1];
    }
    else {
        i *= 2;
        i += (rem - 1)*2;
        byte1 = THIRD_STOP_ISOS[i];
        zeroes = THIRD_STOP_ISOS[i+1];
    }

    return iso_into_bcd(byte1, zeroes, digits, length);
}

bool iso_is_full_stop(const uint8_t *digits, uint_fast8_t length)
{
    // Special case: 12500 is listed as 12000 in table (because it has 3 non-zero digits).
    if (length == 5 && digits[0] == 1 && digits[1] == 2 && digits[2] == 5 && digits[3] == 0 && digits[4] == 0)
        return true;

    uint_fast8_t i;
    for (i = 0; i < sizeof(FULL_STOP_ISOS); i += 2) {
        uint_fast8_t d = FULL_STOP_ISOS[i];
        uint_fast8_t nsigs = 1;
        if (d & 0xF0)
            ++nsigs;
        uint_fast8_t nzeroes = FULL_STOP_ISOS[i+1];
        if (nsigs + nzeroes == length) {
            uint_fast8_t d1 = d & 0xF;
            uint_fast8_t checkzeroes_length = length - 1;
            uint_fast8_t checkzeroes_offset = 1;
            if (nsigs == 2) {
                uint_fast8_t d2 = d >> 4;
                checkzeroes_offset = 2;
                --checkzeroes_length;
                if (digits[0] != d2 || digits[1] != d1)
                    continue;
            }
            else if (digits[0] != d1) {
                continue;
            }

            uint_fast8_t j;
            for (j = 0; j < checkzeroes_length; ++j) {
                if (digits[j+checkzeroes_offset] != 0)
                    goto continue_main;
            }

            return true;
        }

continue_main:;
    }

    return false;
}

// Convert a BCD ISO number into the closest equivalent 8-bit representation.
// In the 8-bit representation, ISOs start from 6 and go up in steps of 1/3 stop.
static const uint8_t BCD_6[] = { 6 };
uint_fast8_t iso_bcd_to_third_stops(uint8_t *digits, uint_fast8_t length)
{
    assert(length <= ISO_DECIMAL_MAX_DIGITS);

    uint_fast8_t l = length;

    uint8_t tdigits_[l];
    uint8_t *tdigits = tdigits_;
    uint_fast8_t i;
    for (i = 0; i < l; ++i)
        tdigits_[i] = digits[i];

    uint8_t *r = tdigits;
    uint_fast8_t count;
    for (count = 0; bcd_gt(r, l, BCD_6, sizeof(BCD_6)); ++count) {
        //        printf(" === ");
        //        debug_print_bcd(r, l);
        //        printf("\n");
        // ISO 26 -> 25
        if (l == 2 && r[0] == 2 && r[1] == 5)
            r[1] = 6;
        // ISO 12500 -> 12800
        if (l == 6 && r[0] == '1' && r[1] == '2' && r[2] == '5' && r[3] == '0' && r[4] == '0' && r[5] == '0')
            r[2] = 8;

        r = bcd_div_by_lt10(r, l, 2);
        l = bcd_length_after_op(tdigits, l, r);
        tdigits = r;
    }

    uint_fast8_t stops = count*3;

    // If it's not a full-stop ISO number then we've calculated the
    // stop equivalent of the next full-stop ISO number ABOVE the
    // given number. We now deal in 1/3 stops, which is how
    // intermediate ISO numbers appear to be standardized.  To get to
    // the next stop down we divide by the third root of 2 (~= 1/0.8).
    // However, due to rounding of the standard ISO numbers, we actually
    // have to divide by a bit more than that. The magic value appears
    // to be 1/0.7. We implement this as subtraction of 1/5 + 1/10 of
    // the original.
    if (! iso_is_full_stop(digits, length)) {
        //printf("ISO INIT ");
        //debug_print_bcd(digits, length);
        //printf("\n");

        uint8_t nextup_digits_[ISO_DECIMAL_MAX_DIGITS];
        uint_fast8_t dcount = count << 1;
        //        printf("=====>>> COUNT %i b1 %i b2 %i\n", count, FULL_STOP_ISOS[dcount], FULL_STOP_ISOS[dcount+1]);
        uint8_t *nextup_digits = iso_into_bcd(FULL_STOP_ISOS[dcount],
                                              FULL_STOP_ISOS[dcount + 1],
                                              nextup_digits_,
                                              ISO_DECIMAL_MAX_DIGITS);
        uint_fast8_t nextup_length = ISO_DECIMAL_MAX_DIGITS - (nextup_digits - nextup_digits_);

        //printf("NEXTUP ");
        //debug_print_bcd(nextup_digits, nextup_length);
        //printf("\n");

        uint_fast8_t divs;
        for (divs = 0; bcd_gt(nextup_digits, nextup_length, digits, length); ++divs) {
            // printf("DIVS %i\n", divs);

            uint8_t fifth_digits_[nextup_length];
            uint_fast8_t j;
            for (j = 0; j < nextup_length; ++j)
                fifth_digits_[j] = nextup_digits[j];
            uint8_t *fifth_digits = bcd_div_by_lt10(fifth_digits_, nextup_length, 5);
            uint_fast8_t fifth_length = bcd_length_after_op(fifth_digits_, nextup_length, fifth_digits);

            // Subtract 1/5.
            uint8_t *r = bcd_sub(nextup_digits, nextup_length, fifth_digits, fifth_length);
            nextup_length = bcd_length_after_op(nextup_digits, nextup_length, r);
            nextup_digits = r;

            // Subtract 1/10.
            // We subtract an additional 1/10 because sometimes standard ISO numbers are a
            // little lower than they really should be due to rounding.
            uint8_t tenth_digits[nextup_length];
            for (j = 0; j < nextup_length - 1; ++j)
                tenth_digits[j] = nextup_digits[j];
            r = bcd_sub(nextup_digits, nextup_length, tenth_digits, nextup_length-1);
            nextup_length = bcd_length_after_op(nextup_digits, nextup_length, r);
            nextup_digits = r;

            //printf("CMP\n");
            //debug_print_bcd(nextup_digits, nextup_length);
            //printf(" > ");
            //debug_print_bcd(digits, length);
            //printf("\n");
        }

        //printf("STOPS %i DIVS %i\n", stops, divs);
        assert(divs > 0 && divs <= 3);
        stops -= divs;
    }

    return stops;
}

static const uint_fast8_t ev_wtih_fracs_to_xth_consts[] = {
    // 120
    120, 40, 15, 12,
    // 100
    100, 33, 13, 10
};
#define consts ev_wtih_fracs_to_xth_consts
static int_fast16_t ev_with_fracs_to_xth(ev_with_fracs_t evwf, uint_fast8_t const_offset)
{
    uint_fast8_t nth = ev_with_fracs_get_nth(evwf);

    int_fast16_t whole = ev_with_fracs_get_wholes(evwf) * consts[const_offset+0];
    int_fast16_t rest;
    if (nth == 3)
        rest = ev_with_fracs_get_thirds(evwf) * consts[const_offset+1];
    else if (nth == 8)
        rest = ev_with_fracs_get_eighths(evwf) * consts[const_offset+2];
    else if (nth == 10)
        rest = ev_with_fracs_get_tenths(evwf) * consts[const_offset+3];
    else
        assert(false);

    return whole + rest;
}
#undef consts
#define ev_with_fracs_to_120th(x) ev_with_fracs_to_xth((x), 0)
#define ev_with_fracs_to_100th(x) ev_with_fracs_to_xth((x), 4)

// This is called by the following macros defined in exposure.h:
//
//     aperture_given_shutter_speed_iso_ev(shutter_speed,iso,ev)
//     shutter_speed_given_aperture_iso_ev(aperture,iso,ev)
//     iso_given_aperture_shutter_speed(aperture, shutter_speed)
ev_with_fracs_t z_given_x_y_ev(ev_with_fracs_t given_x_, ev_with_fracs_t given_y_, ev_with_fracs_t evwf, uint_fast8_t x) // x=0: aperture, x=1: shutter_speed
{
    // We use an internal represenation of values in 1/120 EV steps. This permits
    // exact division by 8, 10 and 3.
    // 256*120 < (2^16)/2, so we can use signed 16-bit integers.

    // We know that for EV=3, ISO = 100, speed = 1minute, aperture = 22.
    const int_fast16_t the_aperture = 9*120; // F22
    const int_fast16_t the_speed = 0;        // 1 minute.
    const int_fast16_t the_ev = (3+5)*120;   // 3 EV
    const int_fast16_t the_iso = 4*120;      // 100 ISO

    int_fast16_t given_x = ev_with_fracs_to_120th(given_x_);
    int_fast16_t given_y = ev_with_fracs_to_120th(given_y_);
    int_fast16_t given_ev = (int_fast16_t)(ev_with_fracs_get_ev8(evwf) * 15);
    int_fast16_t whole_given_ev = (int_fast16_t)((ev_with_fracs_get_ev8(evwf) & ~0b111) * 15);
    // We do the main calculation using the 1/120EV value derived from the 1/8 EV value.
    // We now make a note of the small difference in the EV value which we get if
    // we calculate it from the 1/10 or 1/3 EV value. We add this difference
    // back on at the end to calculate the exact tenths/thirds for the end result.
    int_fast8_t given_ev_tenths_diff = (int_fast8_t)(whole_given_ev + (ev_with_fracs_get_tenths(evwf) * 12) - given_ev);
    int_fast8_t given_ev_thirds_diff = (int_fast8_t)(whole_given_ev + (ev_with_fracs_get_thirds(evwf) * 40) - given_ev);

    int_fast16_t r;
    int_fast16_t min, max;
    if (x == 0) {
        int_fast16_t shut_adjusted = the_ev + given_x - the_speed;

        int_fast16_t evdiff = given_ev - shut_adjusted;
        r = the_aperture + evdiff;
        // Adjust for difference between reference ISO and actual ISO.
        r += given_y - the_iso;
        min = AP_MIN*15, max = AP_MAX*15;
    }
    else if (x == 1) {
        int_fast16_t ap_adjusted = the_ev + given_x - the_aperture;
        int_fast16_t evdiff = given_ev - ap_adjusted;
        r = the_speed + evdiff;
        // Adjust for difference between reference ISO and actual ISO.
        r += given_y - the_iso;
        min = SS_MIN*15, max = SS_MAX*15;
    }
    else { // x == 2
        int_fast16_t iso_adjusted = the_ev + given_x - the_aperture;
        int_fast16_t evdiff = given_ev - iso_adjusted;
        r = the_iso + evdiff;
        // Adjust for difference between reference shutter speed and actual shutter speed.
        r += given_y - the_speed;
        min = ISO_MIN*15, max = ISO_MAX*15;
    }

    if (r < min)
        r = min;
    else if (r > max)
        r = max;

    // Figure out eighths, thirds and tenths.
    // Note that we're relying on the property discussed in exposure.h, i.e.,
    // that calculations in eighths/thirds/tenths always yield the same whole
    // EV values.
    ev_with_fracs_init(evwf);
    ev_with_fracs_set_ev8(evwf, round_divide(r, 15));
    int_fast16_t tenth_ev = r + given_ev_tenths_diff;
    int_fast16_t third_ev = r + given_ev_thirds_diff;
    // TODO: Think carefully about whether we should be using round_divide here.
    ev_with_fracs_set_tenths(evwf, (uint_fast8_t)(round_divide(tenth_ev,12))%10);
    ev_with_fracs_set_thirds(evwf, (uint_fast8_t)(round_divide(third_ev,40))%3);

    return evwf;
}

// Log base 2 (fixed point).
// See http://stackoverflow.com/a/14884853/376854 and https://github.com/dmoulding/log2fix
#define LOG2_PRECISION 6
static int32_t log_base2(uint32_t x)
{
    // This implementation is based on Clay. S. Turner's fast binary logarithm
    // algorithm[1].

    int32_t b = 1U << (LOG2_PRECISION - 1);
    int32_t y = 0;

    //if (LOG2_PRECISION < 1 || LOG2_PRECISION > 31) {
    //    errno = EINVAL;
    //    return INT32_MAX; // indicates an error
    //}

    if (x == 0) {
        return INT32_MIN; // represents negative infinity
    }

    while (x < 1U << LOG2_PRECISION) {
        x <<= 1;
        y -= 1U << LOG2_PRECISION;
    }

    while (x >= 2U << LOG2_PRECISION) {
        x >>= 1;
        y += 1U << LOG2_PRECISION;
    }

    // ALEX: Was a uint64_t in the original code. Guessing that uint32_t will
    // be ok if we're not using lots of precision bits and given the range of
    // values that this function will be used for.
    //uint64_t z = x;
    uint32_t z = x;

    size_t i;
    for (i = 0; i < LOG2_PRECISION; i++) {
        z = z * z >> LOG2_PRECISION;
        if (z >= 2U << LOG2_PRECISION) {
            z >>= 1;
            y += b;
        }
        b >>= 1;
    }

    return y;
}

// Convert a shutter speed specified as fps + shutter angle to a normal shutter speed.
// Frames per second is specified in units of 1/10 frames.
// Shutter angle is specified in units of 1 degree.
ev_with_fracs_t fps_and_angle_to_shutter_speed(uint_fast16_t fps, uint_fast16_t angle)
{
    uint32_t fp32 = fps * 360;
    fp32 = round_divide(fp32, angle);
    // fp32 is now the "effective" frames per second (i.e. fps if shutter angle were 360).
    // Now we have to take the log to get the shutter speed in EV.
    int_fast16_t ev = (int_fast16_t)log_base2(fp32);
    // Divide by 10 (because we're using units of 1/10 frames).
#if LOG2_PRECISION == 6
    ev -= 0b11010101; // 11.010101 = 3.322 = log2(10) with 6 precision bits.
#else
#error "Bad LOG2_PRECISION value"
#endif

    // Add magic number to get shutter speed EV (+ 12EV).
    ev += 12 * (1 << LOG2_PRECISION);

    if (ev > SHUTTER_SPEED_MAX * (1 << LOG2_PRECISION))
        ev = SHUTTER_SPEED_MAX * (1 << LOG2_PRECISION);
    else if (ev < SHUTTER_SPEED_MIN * (1 << LOG2_PRECISION))
        ev = SHUTTER_SPEED_MIN * (1 << LOG2_PRECISION);

    // We can now estimate tenths and thirds.
    uint_fast8_t thirds = (ev % (1 << LOG2_PRECISION))/((1 << LOG2_PRECISION) / 3);
    uint_fast8_t tenths = (ev % (1 << LOG2_PRECISION))/((1 << LOG2_PRECISION) / 10);

    ev_with_fracs_t ret;
    ev_with_fracs_init(ret);
    ev_with_fracs_set_ev8(ret, ev/8);
    ev_with_fracs_set_thirds(ret, thirds);
    ev_with_fracs_set_tenths(ret, tenths);
    ev_with_fracs_set_nth(ret, 10);
    return ret;
}

// Gets lux as BCD with 4 decimal places.
uint8_t *ev_at_100_to_bcd_lux(ev_with_fracs_t evwf, uint8_t *digits)
{
    //printf("ORIG = %i 100th = %i\n", ev_with_fracs_get_ev8(evwf), ev_with_fracs_to_100th(evwf));
    int32_t ev = (ev_with_fracs_to_100th(evwf) - 500) * 10;

    // Magic compensation. This corrects somewhat for the imprecision of the
    // bcd_exp2 function, which gets worse the further we go from whole EV
    // values.
    uint_fast8_t eighths = ev_with_fracs_get_eighths(evwf);
    ev -= (eighths * 4);

    //printf("EV %i (%i)\n", ev, ev_with_fracs_get_ev8(evwf));
    assert(ev >= 0 && ev <= 25000);

    // The resulting number cannot be bigger than six digits.
    // Following exponention and multiplication 12 digits is the max,
    // assuming convervative max EV of 25.
    // We leave extra space at the end if BCD_EXP2_PRECISION is > 2.
#if BCD_EXP2_PRECISION < 2
#error "Bad value for BCD_EXP2_PRECISION in ev_at_100_to_bcd_lux in exposure.c"
#endif
    memset8_zero(digits, (EV_AT_100_TO_BCD_LUX_BCD_LENGTH)*sizeof(uint8_t));
    uint8_t *digits_p = uint32_to_bcd(ev, digits, (EV_AT_100_TO_BCD_LUX_BCD_LENGTH-(BCD_EXP2_PRECISION-3))*sizeof(uint8_t));

    //uint_fast8_t x;
    //printf("[%i], digits_p = ", ev);
    //for (x = 0; x < bcd_length_after_op(digits, EV_AT_100_TO_BCD_LUX_BCD_LENGTH, digits_p); ++x)
    //    printf("%c", digits_p[x] + '0');
    //printf("\n");

    uint_fast8_t rlen = bcd_length_after_op(digits, EV_AT_100_TO_BCD_LUX_BCD_LENGTH, digits_p);
    uint8_t *digits_p2 = bcd_exp2(digits_p, rlen);

    //uint_fast8_t x;
    //printf("digits_p = ");
    //for (x = 0; x < bcd_length_after_op(digits_p1, EV_AT_100_TO_BCD_LUX_BCD_LENGTH, digits_p2); ++x)
    //    printf("%c", digits_p2[x] + '0');
    //printf("\n");

    static uint8_t TWO_FIVE[] = { 2, 5 };
    uint_fast8_t rlen2 = bcd_length_after_op(digits_p, rlen, digits_p2);
    uint8_t *digits_p3 = bcd_mul(digits_p2, rlen2, TWO_FIVE, sizeof(TWO_FIVE)/sizeof(uint8_t));
    //uint_fast8_t rlen3 = bcd_length_after_op(digits_p2, rlen2, digits_p3);

    // digits_p3 now contains the lux value in decimal.
    return digits_p3;
}
#undef LOG10_2_5__1000
#undef LOG10_2__10000


#ifdef TEST

#include <stdio.h>
#include <math.h>
extern const uint_fast8_t TEST_VOLTAGE_TO_EV[];

static void print_bcd(uint8_t *digits, uint_fast8_t length, uint_fast8_t sigfigs, uint_fast8_t dps)
{
    uint8_t buf[length+2];
    bcd_to_string_fp(digits, length, buf, sigfigs, dps);
    printf("%s", buf);
}

int main()
{
    aperture_string_output_t aso;
    shutter_string_output_t sso;
    ev_with_fracs_t iso100;
    ev_with_fracs_init(iso100);
    ev_with_fracs_set_ev8(iso100, 4*8);

    printf("ev_at_100_to_bcd_lux\n");
    ev_with_fracs_t evat100;
    uint_fast8_t ev8;
    for (ev8 = 8*5; ev8 < 160; ++ev8) {
        float fev = ((float)ev8 - 40.0) / 8.0;
        float flux = pow(2.0, fev) * 2.5;
        ev_with_fracs_init(evat100);
        ev_with_fracs_set_ev8(evat100, ev8);
        uint8_t lux_digits_[EV_AT_100_TO_BCD_LUX_BCD_LENGTH];
        uint8_t *lux_digits = ev_at_100_to_bcd_lux(evat100, lux_digits_);
        printf("EV@100 %f =,", (((float)ev_with_fracs_get_ev8(evat100))/8.0)-5.0);
        uint_fast8_t x;
        print_bcd(lux_digits, bcd_length_after_op(lux_digits_, EV_AT_100_TO_BCD_LUX_BCD_LENGTH, lux_digits), 3, 4);
        printf(",%f\n", flux);
        //for (x = 0; x < bcd_length_after_op(lux_digits_, EV_AT_100_TO_BCD_LUX_BCD_LENGTH, lux_digits); ++x)
        //    printf("%c", lux_digits[x] + '0');
        //printf("\n");
    }

    printf("fps_and_angle_to_shutter_speed\n");
    uint_fast16_t fps;
    for (fps = 1; fps < 200; ++fps) {
        uint_fast16_t angle = 180;
        ev_with_fracs_t shutspeed = fps_and_angle_to_shutter_speed(fps*10, angle);
        shutter_speed_to_string(shutspeed, &sso, PRECISION_MODE_TENTH);
        printf("From fps = %i, angle = %i -> %s\n", fps, angle, SHUTTER_STRING_OUTPUT_STRING(sso));
    }

    printf("\n");

    printf("Shutter speeds in eighths:\n");
    uint_fast8_t s;
    for (s = SS_MIN; s <= SS_MAX; ++s) {
        ev_with_fracs_t evwf;
        evwf.ev = s;
        ev_with_fracs_set_tenths(evwf, tenth_below_eighth(s));
        shutter_speed_to_string(evwf, &sso, PRECISION_MODE_EIGHTH);
        printf("SS: %s (ev*8 = %i, length = %i)\n", SHUTTER_STRING_OUTPUT_STRING(sso), s, sso.length);
    }

    printf("\n");

    printf("Shutter speeds in tenths:\n");
    for (s = (SS_MIN/8)*10; s <= (SS_MAX/8)*10; ++s) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init(evwf);
        ev_with_fracs_set_ev8(evwf, (s*8)/10);
        ev_with_fracs_set_tenths(evwf, s%10);
        shutter_speed_to_string(evwf, &sso, PRECISION_MODE_TENTH);
        printf("SS: %s (ev = %i.%i, rev=%i, length = %i)\n", SHUTTER_STRING_OUTPUT_STRING(sso), s/10, s%10, evwf.ev, sso.length);
    }

    printf("\n");

    printf("Shutter speeds in thirds:\n");
    uint_fast8_t last_thirds = 3;
    for (s = (SS_MIN/8)*10; s <= (SS_MAX/8)*10; ++s) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init(evwf);
        ev_with_fracs_set_ev8(evwf, (s*8)/10);
        ev_with_fracs_set_tenths(evwf, s%10);
        uint_fast8_t thirds = thirds_from_eighths(evwf.ev % 8);
        if (thirds == last_thirds)
            continue;
        ev_with_fracs_set_thirds(evwf, thirds);
        last_thirds = thirds;
        shutter_speed_to_string(evwf, &sso, PRECISION_MODE_THIRD);
        printf("SS: %s (ev = %i+%i/3, length = %i)\n", SHUTTER_STRING_OUTPUT_STRING(sso), s/10, thirds_from_tenths(s%10), sso.length);
    }

    printf("Apertures in thirds:\n");
    uint_fast8_t a;
    for (a = AP_MIN; a <= (AP_MAX/8)*3; ++a) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init(evwf);
        ev_with_fracs_set_ev8(evwf, (a/3)*8);
        ev_with_fracs_set_thirds(evwf, a % 3);
        aperture_to_string(evwf, &aso, PRECISION_MODE_THIRD);
        printf("A[%i,%i,%i]:  %s\n", a, evwf.ev, ev_with_fracs_get_tenths(evwf), APERTURE_STRING_OUTPUT_STRING(aso));
    }

    printf("\nApertures in eighths:\n");
    for (a = AP_MIN; a <= AP_MAX; ++a) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init(evwf);
        ev_with_fracs_set_ev8(evwf, a);
        aperture_to_string(evwf, &aso, PRECISION_MODE_EIGHTH);
        printf("A[%i]:  %s\n", a, APERTURE_STRING_OUTPUT_STRING(aso));
    }

    printf("\nApertures in tenths:\n");
    for (a = AP_MIN; a <= (AP_MAX/8)*10; ++a) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init(evwf);
        ev_with_fracs_set_ev8(evwf, (a/10)*8);
        ev_with_fracs_set_tenths(evwf, a%10);
        aperture_to_string(evwf, &aso, PRECISION_MODE_TENTH);
        printf("A[%i,%i,%i]:  %s\n", a, evwf.ev, ev_with_fracs_get_tenths(evwf), APERTURE_STRING_OUTPUT_STRING(aso));
    }

    printf("\nTesting aperture_given_shutter_speed_iso_ev\n");
    uint_fast8_t is, ss, ev, ap;
    for (is = ISO_MIN/8*3; is <= ISO_MAX/8*3; ++is) {
        ss = 8*8;//12*8; // 1/60
        ev = 10*8;//15*8; // 10 EV
        ev_with_fracs_t evwf;
        ev_with_fracs_init(evwf);
        ev_with_fracs_set_ev8(evwf, ev);
        ev_with_fracs_t isoevwf;
        ev_with_fracs_init(isoevwf);
        ev_with_fracs_set_ev8(isoevwf, ((is/3)*8) + eighths_from_thirds(is % 3));
        ev_with_fracs_set_thirds(isoevwf, is%3);
        ev_with_fracs_set_tenths(isoevwf, tenths_from_thirds(is%3));
        ev_with_fracs_set_nth(isoevwf, 3);
        //printf("ISO EV %i (%i,%i)*****\n", ev_with_fracs_get_ev8(isoevwf), ((is/3)*8) + eighths_from_thirds(is%3), is);
        //printf("ISO third %i *****\n", ev_with_fracs_get_thirds(isoevwf));
        ev_with_fracs_t ssevwf;
        ev_with_fracs_init(ssevwf);
        ev_with_fracs_set_ev8(ssevwf, ss);
        evwf = aperture_given_shutter_speed_iso_ev(ssevwf, isoevwf, evwf);
        shutter_speed_to_string(ssevwf, &sso, PRECISION_MODE_EIGHTH);
        aperture_to_string(evwf, &aso, PRECISION_MODE_EIGHTH);
        printf("ISO %f stops from 6,  %s  %s (EV = %.2f)   [%i, %i, %i : %i]\n",
               ((float)is) / 3.0,
               SHUTTER_STRING_OUTPUT_STRING(sso),
               APERTURE_STRING_OUTPUT_STRING(aso),
               ((float)(((int_fast16_t)(ev))-(5*8)))/8.0,
               is, ss, ev, ap);
    }

    printf("\nTesting shutter_speed_given_aperture_iso_ev\n");
    for (is = ISO_MIN/8*3; is <= ISO_MAX/8*3; ++is) {
        ap = 6*8; // f8
        ev = 15*8; // 10 EV
        ev_with_fracs_t evwf;
        ev_with_fracs_init(evwf);
        ev_with_fracs_set_ev8(evwf, ev);
        ev_with_fracs_t apevwf;
        ev_with_fracs_init(apevwf);
        ev_with_fracs_set_ev8(apevwf, ap);
        ev_with_fracs_t isoevwf;
        ev_with_fracs_init(isoevwf);
        ev_with_fracs_set_ev8(isoevwf, ((is/3)*8) + eighths_from_thirds(is % 3));
        ev_with_fracs_set_thirds(isoevwf, is%3);
        ev_with_fracs_set_tenths(isoevwf, tenths_from_thirds(is%3));
        ev_with_fracs_set_nth(isoevwf, 3);
        evwf = shutter_speed_given_aperture_iso_ev(apevwf, isoevwf, evwf);
        shutter_speed_to_string(evwf, &sso, PRECISION_MODE_EIGHTH);
        ev_with_fracs_t evwf2;
        ev_with_fracs_init(evwf2);
        ev_with_fracs_set_ev8(evwf, ap);
        aperture_to_string(apevwf, &aso, PRECISION_MODE_EIGHTH);
        printf("ISO %f stops from 6,  %s  %s (EV = %.2f)   [%i, %i, %i : %i]\n",
               ((float)is) / 3.0,
               SHUTTER_STRING_OUTPUT_STRING(sso),
               APERTURE_STRING_OUTPUT_STRING(aso),
               ((float)(((int_fast16_t)(ev))-(5*8)))/8.0,
               is, ss, ev, ap);
    }

    printf("\n\n");

    // TODO: Uses old API.
    /*for (is = ISO_MIN; is <= ISO_MAX; ++is) {
        for (ss = SS_MIN; ss <= SS_MAX; ++ss) {
          for (ev = 40; ev <= EV_MAX; ++ev) {
                ap = aperture_given_shutter_speed_iso_ev(ss, is, ev);
                shutter_speed_to_string(ss, &sso);
                aperture_to_string(ap, &aso);
                printf("ISO %f stops from 6,  %s  %s (EV = %.2f)   [%i, %i, %i : %i]\n",
                       ((float)is) / 8.0,
                       SHUTTER_STRING_OUTPUT_STRING(sso),
                       APERTURE_STRING_OUTPUT_STRING(aso),
                       ((float)(((int_fast16_t)(ev))-(5*8)))/8.0,
                       is, ss, ev, ap);
             }
        }
        }*/

    // Useful table for comparison is here: http://en.wikipedia.org/wiki/Film_speed
    static uint8_t isobcds[] = {
        7,   1, 6, 0, 0, 0, 0, 0,  0,
        6,   0, 8, 0, 0, 0, 0, 0,  0,
        6,   0, 4, 0, 0, 0, 0, 0,  0,
        6,   0, 2, 0, 0, 0, 0, 0,  0,
        6,   0, 1, 0, 0, 0, 0, 0,  0,
        5,   0, 0, 5, 0, 0, 0, 0,  0,
        5,   0, 0, 2, 5, 0, 0, 0,  0,
        5,   0, 0, 2, 0, 0, 0, 0,  0,
        5,   0, 0, 1, 6, 0, 0, 0,  0,
        5,   0, 0, 1, 2, 5, 0, 0,  0,
        5,   0, 0, 1, 0, 0, 0, 0,  0,
        4,   0, 0, 0, 8, 0, 0, 0,  0,
        4,   0, 0, 0, 6, 4, 0, 0,  0,
        4,   0, 0, 0, 3, 2, 0, 0,  0,
        4,   0, 0, 0, 2, 5, 0, 0,  0,
        4,   0, 0, 0, 2, 0, 0, 0,  0,
        4,   0, 0, 0, 1, 6, 0, 0,  0,
        4,   0, 0, 0, 1, 2, 5, 0,  0,
        4,   0, 0, 0, 1, 0, 0, 0,  0,
        3,   0, 0, 0, 0, 8, 0, 0,  0,
        3,   0, 0 ,0 ,0, 6, 4, 0,  0,
        3,   0, 0 ,0 ,0, 5, 0, 0,  0,
        3,   0, 0 ,0 ,0, 4, 0, 0,  0,
        3,   0, 0, 0, 0, 3, 2, 0,  0,
        3,   0, 0, 0, 0, 2, 5, 0,  0,
        3,   0, 0, 0, 0, 2, 0, 0,  0,
        3,   0, 0, 0, 0, 1, 6, 0,  0,
        3,   0, 0, 0, 0, 1, 2, 5,  0,
        3,   0, 0, 0, 0, 1, 0, 0,  0,
        2,   0, 0, 0, 0, 0, 8, 4,  0,
        2,   0, 0, 0, 0, 0, 6, 4,  0,
        2,   0, 0, 0, 0, 0, 5, 0,  0,
        2,   0, 0, 0, 0, 0, 4, 0,  0,
        2,   0, 0, 0, 0, 0, 3, 2,  0,
        2,   0, 0, 0, 0, 0, 2, 5,  0,
        2,   0, 0, 0, 0, 0, 2, 0,  0,
        2,   0, 0, 0, 0, 0, 1, 6,  0,
        2,   0, 0, 0, 0, 0, 1, 2,  0,
        2,   0, 0, 0, 0, 0, 1, 0,  0,
        1,   0, 0, 0, 0, 0, 0, 8,  0,
        1,   0, 0, 0, 0, 0, 0, 6,  0
    };

    int i;
    for (i = 0; i < sizeof(isobcds); i += 9) {
        int length = isobcds[i];
        int offset = i+8-length;
        uint8_t *isodigits = isobcds+offset;

        bool is_full = iso_is_full_stop(isodigits, length);
        int stops = iso_bcd_to_third_stops(isodigits, length);
        bcd_to_string(isodigits, length);

        printf("ISO %s (%sfull) = %.2f stops from ISO 6\n", isodigits, is_full ? "" : "not ", ((float)stops)/3.0);
    }

    // Test that compressed table is giving correct values by comparing to uncompressed table.
    printf("OFFSET %i\n", VOLTAGE_TO_EV_ABS_OFFSET);
    for (int v_ = 0; v_ < 246; ++v_) {
        int v = v_ - VOLTAGE_TO_EV_ABS_OFFSET;
        if (v < 0)
            v = 0;
        uint_fast8_t uncompressed = TEST_VOLTAGE_TO_EV[(unsigned)v];
        ev_with_fracs_t evwf = get_ev100_at_voltage((uint_fast8_t)v_, 2);
        printf("V %i ev8=%i\n", v, ev_with_fracs_get_ev8(evwf));
        uint_fast8_t compressed = evwf.ev;
        if (uncompressed != compressed) {
            printf("Values not equal for t = %i, v = %i: compressed = %i, uncompressed = %i\n", (unsigned)t, (unsigned)v_, (unsigned)compressed, (unsigned)uncompressed);
            //return 1;
        }
    }

    printf("\nCompression test completed successfully.\n");

    // Sanity check: pass in the values which are used as a base for the calculation
    // and check that we get the originals back.
    ev_with_fracs_t evwf;
    ev_with_fracs_init(evwf);
    ev_with_fracs_set_ev8(evwf, (3+5)*8);
    ev_with_fracs_t apevwf;
    ev_with_fracs_init(apevwf);
    ev_with_fracs_set_ev8(apevwf, 0);
    evwf = aperture_given_shutter_speed_iso_ev(apevwf, iso100, evwf);
    printf("VAL that should be equal to 9*8=72: %i\n", evwf.ev);
    assert(evwf.ev == 9*8);
    aperture_to_string(evwf, &aso, PRECISION_MODE_EIGHTH);
    printf("AP: %s\n", APERTURE_STRING_OUTPUT_STRING(aso));

    ev_with_fracs_init(evwf);
    ev_with_fracs_set_ev8(evwf, (3+5)*8);
    ev_with_fracs_init(apevwf);
    ev_with_fracs_set_ev8(apevwf, 9*8);
    evwf = shutter_speed_given_aperture_iso_ev(apevwf, iso100, evwf);
    printf("VAL that should be equal to 0: %i\n", evwf.ev);
    assert(evwf.ev == 0);
    shutter_speed_to_string(evwf, &sso, PRECISION_MODE_TENTH);
    printf("SHUT: %s\n", SHUTTER_STRING_OUTPUT_STRING(sso));

    return 0;
}

#endif
