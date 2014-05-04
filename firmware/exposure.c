// Apertures are represented as unsigned 8-bit quantities
// from 1.0 to 32 in 1/8-stop steps.
//
// Shutter speeds are represented as unsigned 8-bit quantities
// from 1 minute to 1/16000 in 1/8-stop steps. Some key points
// on the scale are defined in exposure.h

#include <readbyte.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <exposure.h>
#include <bcd.h>
#include <readbyte.h>
#include <state.h>
#include <tables.h>
#ifdef TEST
#include <stdio.h>
#endif

// See comments in calculate_tables.py for info on the way
// temp/voltage are encoded.
// This is explicitly implemented without using division or multiplcation,
// since the attiny doesn't have hardware division or multiplication.
// gcc would probably do most of these optimizations automatically, but since
// this code really definitely needs to run quickly, I'm doing it explicitly here.
//
// 'voltage' is in 1/256ths of the reference voltage.
ev_with_tenths_t get_ev100_at_temperature_voltage(uint8_t temperature, uint8_t voltage, uint8_t op_amp_resistor_stage)
{
    const uint8_t *ev_abs = NULL, *ev_diffs = NULL, *ev_bitpatterns = NULL, *ev_tenths = NULL;
#define ASSIGN(n) (ev_abs = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_ABS,                 \
                   ev_diffs = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_DIFFS,             \
                   ev_bitpatterns = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_BITPATTERNS, \
                   ev_tenths = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_TENTHS)
    switch (op_amp_resistor_stage) {
    case 1: ASSIGN(1); break;
#if NUM_OP_AMP_RESISTOR_STAGES > 1
    case 2: ASSIGN(2); break;
#if NUM_OP_AMP_RESISTOR_STAGES > 2
    case 3: ASSIGN(3); break;
#if NUM_OP_AMP_RESISTOR_STAGES > 3
    case 4: ASSIGN(4); break;
#if NUM_OP_AMP_RESISTOR_STAGES > 4
    case 5: ASSIGN(5); break;
#if NUM_OP_AMP_RESISTOR_STAGES > 5
#error "Too many op amp stages"
#endif
#endif
#endif
#endif
#endif
    }
#undef ASSIGN

    ev_with_tenths_t ret;

    if (voltage < VOLTAGE_TO_EV_ABS_OFFSET)
        voltage = 0;
    else
        voltage -= VOLTAGE_TO_EV_ABS_OFFSET;

    uint8_t v16 = voltage >> 4;

    uint8_t absi = v16;
    uint8_t bits_to_add = (voltage & 15) + 1; // (voltage % 16) + 1

    uint8_t bit_pattern_indices = pgm_read_byte(ev_diffs + absi);
    uint8_t bits1 = pgm_read_byte(ev_bitpatterns + (bit_pattern_indices >> 4));
    uint8_t bits2 = pgm_read_byte(ev_bitpatterns + (bit_pattern_indices & 0x0F));
    if (bits_to_add < 8)
        bits1 &= 0xFF << (8 - bits_to_add);
    if (bits_to_add < 16)
        bits2 &= 0xFF << (16 - bits_to_add);

    // See http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
    uint8_t r = pgm_read_byte(ev_abs + absi);
    for (; bits1; ++r)
        bits1 &= bits1 - 1;
    for (; bits2; ++r)
        bits2 &= bits2 - 1;

    // Compensate for effect of ambient temperature.
    int8_t adj = TEMP_EV_ADJUST_AT_T0;
    uint8_t i;
    for (i = 0; i < TEMP_EV_ADJUST_CHANGE_TEMPS_LENGTH; ++i) {
        if (temperature >= pgm_read_byte(TEMP_EV_ADJUST_CHANGE_TEMPS + i))
            --adj;
    }
    int16_t withcomp = r;// + adj;
    if (withcomp < 0)
        ret.ev = 0;
    else if (withcomp > 255)
        ret.ev = 255;
    else
        ret.ev = (uint8_t)withcomp;

    // Calculate tenths.
    uint8_t tenths_bit = pgm_read_byte(ev_tenths + (voltage >> 3));
    uint8_t eighths = voltage & 0b111;
    tenths_bit >>= eighths;
    tenths_bit &= 1;
    ret.tenths = eighths;
    if (eighths > 3)
        ++ret.tenths;
    ret.tenths += tenths_bit;

    return ret;
}

void shutter_speed_to_string(uint8_t speed, shutter_string_output_t *eso)
{
    if (speed > SS_MAX)
        speed = SS_MAX;

    uint16_t bytei = (uint16_t)speed;
    bytei = (bytei << 1) + (bytei >> 1);

    uint8_t shift = (speed & 1) << 2;

    uint8_t i, j, nibble;
    uint8_t previous = 0;
    bool already_got_slash = false;
    for (i = 0, j = 0; i < 5; ++j, ++i, shift ^= 4) {
        nibble = (pgm_read_byte(&SHUTTER_SPEEDS[bytei]) >> shift) & 0xF;
        if (nibble == 0)
            break;

        uint8_t c = pgm_read_byte(&SHUTTER_SPEEDS_BITMAP[nibble]);
        if ((c == '+' || c == '-') && !already_got_slash) {
            eso->chars[j++] = 'S';
            eso->chars[j] = c;
        }
        else if (c == '/' && (!previous || previous == '+' || previous == '-')) {
            eso->chars[j++] = '1';
            eso->chars[j] = c;
        }
        else if (c == 'A') {
            eso->chars[j++] = '1';
            eso->chars[j] = '6';
        }
        else if (c == 'B') {
            eso->chars[j++] = '3';
            eso->chars[j] = '2';
        }
        else {
            eso->chars[j] = c;
        }

        if (c == '/')
            already_got_slash = true;

        bytei += ((shift & 4) >> 2);

        previous = c;
    }

    // Add any required trailing zeros. TODO: could probably get rid of the conditionals.
    if (speed >= SS_1000TH)
        eso->chars[j++] = '0';
    if (speed >= SS_10000TH)
        eso->chars[j++] = '0';

    // If it's 1 minute, add the trailing M.
    if (speed == 0)
        eso->chars[j++] = 'M';
    // Add trailing S if required.
    else if (speed <= 51 && !already_got_slash)
        eso->chars[j++] = 'S';

    // Add '\0' termination.
    eso->chars[j] = '\0';

    eso->length = j;
}

void aperture_to_string(ev_with_tenths_t apev, aperture_string_output_t *aso, precision_mode_t precision_mode)
{
    // We do everything in 16-bit fixed point (three decimal places).
    // Convert into common units of 1/80th of an EV.
    int16_t aperture;
    if (precision_mode == PRECISION_MODE_TENTH) {
        aperture = ((apev.ev & ~0b111) * 10) + (apev.tenths * 8);
    }
    else {
        aperture = apev.ev * 10;
    }

    uint16_t r = 1000;
    while (aperture > 0) {
        uint16_t d1 = r/10;
        uint16_t d2 = r/100;
        uint16_t d3 = r/1000;

        if (aperture > 80 || precision_mode == PRECISION_MODE_FULL) {
            r += 4*d1 + 1*d2 + 4*d3;
            aperture -= 80;
        }
        else if (precision_mode == PRECISION_MODE_HALF) {
            r += 1*d1 + 8*d2 + 9*d3;
            aperture -= 40;
        }
        else if (precision_mode == PRECISION_MODE_THIRD) {
            r += 1*d1 + 2*d2 + 2*d3;
            aperture -= 27; // TODO: Consider if this is consistent with third/tenth conversions elsewhere.
        }
        else if (precision_mode == PRECISION_MODE_QUARTER) {
            r += 0*d1 + 9*d2 + 0*d3;
            aperture -= 20;
        }
        else if (precision_mode == PRECISION_MODE_EIGHTH) {
            r += 0*d1 + 4*d2 + 4*d3;
            aperture -= 10;
        }
        else if (precision_mode == PRECISION_MODE_TENTH) {
            r += (d2 << 1) + d2 + (d3 << 2) + d3;
            aperture -= 8;
        }
    }

    //printf("R: %i\n", r);

    uint8_t digits_[5];
    
    // Rounding.
    uint8_t *digits = uint16_to_bcd(r, digits_, sizeof(digits_));
    uint8_t len = bcd_length_after_op(digits_, sizeof(digits_), digits);
   
    uint8_t i = len - 2 + (precision_mode == PRECISION_MODE_TENTH);
    if (digits[i] >= 5) {
        --i;
        ++(digits[i]);
        while (digits[i] >= 10 && i > 0) {
            digits[i] -= 10;
            --i;
            ++(digits[i]);
        }
    }
    // Should never need to add additional digit. Truncate in this case.
    if (digits[0] > 9)
        digits[0] = 9;

    // Add decimal point.
    uint8_t o = len-4;
    if (precision_mode == PRECISION_MODE_TENTH) {
        // Two decimal places.
        digits[3+o] = digits[2+o];
        digits[2+o] = digits[1+o];
        digits[1+o] = '.';
        aso->length = 4 + o;
        //        printf("ASOL* %i\n", aso->length);
    }
    else {
        // One decimal place.
        digits[2+o] = digits[1+o];
        digits[1+o] = '.';
        aso->length = 3 + o;
    }

    // Copy over and convert to ASCII chars.
    //    printf("ASOL* %i\n", aso->length);
    for (i = 0; i < aso->length; ++i) {
        if (digits[i] == '.')
            aso->chars[i] = '.';
        else
            aso->chars[i] = digits[i] + '0';
    }
    aso->chars[i] = '\0';
}

// Table storing full-stop ISO numbers.
// Each pair of bytes is:
//     (i)  big-endian 4-bit BCD rep of first two digits,
//     (ii) number of trailing zeroes.
static const uint8_t FULL_STOP_ISOS[] PROGMEM = {
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

static uint8_t *full_stop_iso_into_bcd(uint8_t byte1, uint8_t zeroes, uint8_t *digits, uint8_t length)
{
    uint8_t i, z;
    for (i = length-1, z = 0; z < zeroes; --i, ++z) {
        digits[i] = 0;
    }

    digits[i] = byte1 & 0x0F;
    if (byte1 & 0xF0) {
        --i;
        digits[i] = byte1 >> 4;
        // Special case for 12500.
        if (byte1 == ((1 << 4) | 2) && zeroes == 3)
            digits[length-3] = 5;
        return digits + length - zeroes - 2;
    }
    else {
        return digits + length - zeroes - 1;
    }
}

bool iso_is_full_stop(const uint8_t *digits, uint8_t length)
{
    // Special case: 12500 is listed as 12000 in table (because it has 3 non-zero digits).
    if (length == 5 && digits[0] == 1 && digits[1] == 2 && digits[2] == 5 && digits[3] == 0 && digits[4] == 0)
        return true;

    uint8_t i;
    for (i = 0; i < sizeof(FULL_STOP_ISOS); i += 2) {
        uint8_t d = pgm_read_byte(&FULL_STOP_ISOS[i]);
        uint8_t nsigs = 1;
        if (d & 0xF0)
            ++nsigs;
        uint8_t nzeroes = pgm_read_byte(&FULL_STOP_ISOS[i+1]);
        if (nsigs + nzeroes == length) {
            uint8_t d1 = d & 0xF;
            uint8_t checkzeroes_length = length - 1;
            uint8_t checkzeroes_offset = 1;
            if (nsigs == 2) {
                uint8_t d2 = d >> 4;
                checkzeroes_offset = 2;
                --checkzeroes_length;
                if (digits[0] != d2 || digits[1] != d1)
                    continue;
            }
            else if (digits[0] != d1) {
                continue;
            }

            uint8_t j;
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
// In the 8-bit representation, ISOs start from 6 and go up in steps of 1/8 stop.
static const uint8_t BCD_6[] = { 6 };
uint8_t iso_bcd_to_stops(uint8_t *digits, uint8_t length)
{
    assert(length <= ISO_DECIMAL_MAX_DIGITS);

    uint8_t l = length;

    uint8_t tdigits_[l];
    uint8_t *tdigits = tdigits_;
    uint8_t i;
    for (i = 0; i < l; ++i)
        tdigits_[i] = digits[i];

    uint8_t *r = tdigits;
    uint8_t count;
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

    uint8_t stops = count*8;

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
        uint8_t nextup_digits_[ISO_DECIMAL_MAX_DIGITS];
        uint8_t dcount = count << 1;
        //        printf("=====>>> COUNT %i b1 %i b2 %i\n", count, FULL_STOP_ISOS[dcount], FULL_STOP_ISOS[dcount+1]);
        uint8_t *nextup_digits = full_stop_iso_into_bcd(pgm_read_byte(&FULL_STOP_ISOS[dcount]),
                                                        pgm_read_byte(&FULL_STOP_ISOS[dcount + 1]),
                                                        nextup_digits_,
                                                        ISO_DECIMAL_MAX_DIGITS);
        uint8_t nextup_length = ISO_DECIMAL_MAX_DIGITS - (nextup_digits - nextup_digits_);

        //printf("NEXTUP ");
        //debug_print_bcd(nextup_digits, nextup_length);
        //printf("\n");

        uint8_t divs;
        for (divs = 1; bcd_gt(nextup_digits, nextup_length, digits, length); ++divs) {
            // printf("DIVS %i\n", divs);

            uint8_t fifth_digits_[nextup_length];
            uint8_t j;
            for (j = 0; j < nextup_length; ++j)
                fifth_digits_[j] = nextup_digits[j];
            uint8_t *fifth_digits = bcd_div_by_lt10(fifth_digits_, nextup_length, 5);
            uint8_t fifth_length = bcd_length_after_op(fifth_digits_, nextup_length, fifth_digits);

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
        
        // Translate 1/3 stops to 1/8 stops -- yuck!
        assert(divs > 0 && divs <= 3);
        switch (divs) {
        case 1: {
            stops -= 3;
        } break;
        case 2: {
            stops -= 5;
        } break;
        case 3: {
            stops -= 8;
        }
        }
    }

    return stops;
}

// We represent ISO in 1/8 stops, with 0 as ISO 6.
// This is called by the following macros defined in exposure.h:
//
//     aperture_given_shutter_speed_iso_ev(shutter_speed,iso,ev)
//     shutter_speed_given_aperture_iso_ev(aperture,iso,ev)
ev_with_tenths_t x_given_y_iso_ev(uint8_t given_x_, uint8_t iso_, ev_with_tenths_t evwt, uint8_t x) // x=0: aperture, x=1: shutter_speed
{
    // We know that for EV=3, ISO = 100, speed = 1minute, aperture = 22.
    int16_t the_aperture = 9*8; // F22
    int16_t the_speed = 0;      // 1 minute.
    int16_t the_ev = ((3+5)*8); // 3 EV
    int16_t the_iso = 4*8;      // 100 ISO

    int16_t given_x = (int16_t)given_x_;
    int16_t given_iso = (int16_t)iso_;
    // We remove the fractional component and add it back at the end.
    // This way we can do the exact calculation for both eighths and tenths.
    int16_t given_ev = (int16_t)evwt.ev & ~0b111;

    int16_t r;
    int16_t min, max;
    if (x == 1) {
        int16_t apdiff = given_x - the_aperture;
        the_ev += apdiff;

        int16_t evdiff = given_ev - the_ev;
        the_speed += evdiff;
        r = the_speed;
        min = SS_MIN, max = SS_MAX;
    }
    else { // x == 0
        int16_t shutdiff = given_x - the_speed;
        the_ev += shutdiff;

        int16_t evdiff = given_ev - the_ev;
        the_aperture += evdiff;
        r = the_aperture;
        min = AP_MIN, max = AP_MAX;
    }

    r += given_iso - the_iso;
    //printf("R: %i (%i - %i)\n", r, given_iso, the_iso);

    // Add back fractional eighths.
    int16_t rr = r + (evwt.ev & 0b111);

    // Add back tenths.
    uint8_t tenths = 0;
    if (r > 0) {
        tenths = (uint8_t)(r & 0b111);
        if (tenths > 3)
            ++tenths;
        tenths += evwt.tenths;
        // Note that we don't need to add 1 to the main value because
        // that will already have been taken care of when we added
        // the fractional eighths back to rr.
        if (tenths > 9)
            tenths -= 10;
    }
    
    if (rr < min)
        rr = min;
    else if (r > max)
        rr = max;

    ev_with_tenths_t ret;
    ret.ev = (uint8_t)rr;
    ret.tenths = tenths;

    return ret;
}

#ifdef TEST

#include <stdio.h>
extern const uint8_t TEST_VOLTAGE_TO_EV[];

int main()
{
    shutter_string_output_t sso;
    /*uint8_t s;
    for (s = SS_MIN; s <= SS_MAX; ++s) {
        shutter_speed_to_string(s, &sso);
        printf("SS: %s (length = %i)\n", SHUTTER_STRING_OUTPUT_STRING(sso), sso.length);
    }

    printf("\n");*/

    uint8_t a;
    aperture_string_output_t aso;
    for (a = (AP_MIN/8)*10; a <= (AP_MAX/8)*10; ++a) {
        ev_with_tenths_t xx;
        xx.ev = (a/10)*8;
        xx.tenths = a % 10;
        aperture_to_string_test(xx, &aso, PRECISION_MODE_TENTH);
        printf("A:  %s\n", APERTURE_STRING_OUTPUT_STRING(aso));
    }

    return 0;

    printf("\nTesting aperture_given_shutter_speed_iso_ev\n");
    uint8_t is, ss, ev, ap;
    for (is = ISO_MIN; is <= ISO_MAX; ++is) {
        ss = 12*8; // 1/60
        ev = 15*8; // 10 EV
        ev_with_tenths_t evwt;
        evwt.ev = ev;
        evwt.tenths = 0;
        evwt = aperture_given_shutter_speed_iso_ev(ss, is, evwt);
        uint8_t ap = evwt.ev;
        shutter_speed_to_string(ss, &sso);
        aperture_to_string(evwt, &aso, PRECISION_MODE_EIGHTH);
        printf("ISO %f stops from 6,  %s  %s (EV = %.2f)   [%i, %i, %i : %i]\n",
               ((float)is) / 8.0,
               SHUTTER_STRING_OUTPUT_STRING(sso),
               APERTURE_STRING_OUTPUT_STRING(aso),
               ((float)(((int16_t)(ev))-(5*8)))/8.0,
               is, ss, ev, ap);
    }
    printf("\nTesting shutter_speed_given_aperture_iso_ev\n");
    for (is = ISO_MIN; is <= ISO_MAX; ++is) {
        ap = 6*8; // f8
        ev = 15*8; // 10 EV
        ev_with_tenths_t evwt;
        evwt.ev = ev;
        evwt.tenths = 0;
        evwt = shutter_speed_given_aperture_iso_ev(ap, is, evwt);
        uint8_t ss = evwt.ev;
        shutter_speed_to_string(ss, &sso);
        aperture_to_string(evwt, &aso, PRECISION_MODE_EIGHTH);
        printf("ISO %f stops from 6,  %s  %s (EV = %.2f)   [%i, %i, %i : %i]\n",
               ((float)is) / 8.0,
               SHUTTER_STRING_OUTPUT_STRING(sso),
               APERTURE_STRING_OUTPUT_STRING(aso),
               ((float)(((int16_t)(ev))-(5*8)))/8.0,
               is, ss, ev, ap);
    }

    printf("\n\n");

    /*for (is = ISO_MIN; is <= ISO_MAX; ++is) {
        for (ss = SS_MIN; ss <= SS_MAX; ++ss) {
          for (ev = 40; ev <= EV_MAX; ++ev) {
                ap = aperture_given_shutter_speed_iso_ev(ss, is, ev);
                shutter_speed_to_string(ss, &sso);
                aperture_to_string(ap, &aso, PRECISION_MODE_EIGHTH);
                printf("ISO %f stops from 6,  %s  %s (EV = %.2f)   [%i, %i, %i : %i]\n",
                       ((float)is) / 8.0,
                       SHUTTER_STRING_OUTPUT_STRING(sso),
                       APERTURE_STRING_OUTPUT_STRING(aso),
                       ((float)(((int16_t)(ev))-(5*8)))/8.0,
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
        int stops = iso_bcd_to_stops(isodigits, length);
        bcd_to_string(isodigits, length);

        printf("ISO %s (%sfull) = %.2f stops from ISO 6\n", isodigits, is_full ? "" : "not ", ((float)stops)/8.0);
    }

    // Test that compressed table is giving correct values by comparing to uncompressed table.
    printf("OFFSET %i\n", VOLTAGE_TO_EV_ABS_OFFSET);
    uint8_t t = 203; // ~=30C; should track reference_temperature in calculate_tables.py
    for (int v_ = 0; v_ < 246; ++v_) {
        int v = v_ - VOLTAGE_TO_EV_ABS_OFFSET;
        if (v < 0)
            v = 0;
        uint8_t uncompressed = pgm_read_byte(&TEST_VOLTAGE_TO_EV[(unsigned)v]);
        ev_with_tenths_t evwt = get_ev100_at_temperature_voltage(t, (uint8_t)v_, 2);
        uint8_t compressed = evwt.ev;
        if (uncompressed != compressed) {
            printf("Values not equal for t = %i, v = %i: compressed = %i, uncompressed = %i\n", (unsigned)t, (unsigned)v_, (unsigned)compressed, (unsigned)uncompressed);
                        return 1;
        }
    }

    printf("\nCompression test completed successfully.\n");

    // Sanity check: pass in the values which are used as a base for the calculation
    // and check that we get the originals back.
    ev_with_tenths_t evwt;
    evwt.ev = (3+5)*8;
    evwt.tenths = 0;
    evwt = aperture_given_shutter_speed_iso_ev(0, 4*8, evwt);
    printf("VAL that should be equal to 9*8=72: %i\n", evwt.ev);
    assert(evwt.ev == 9*8);
    aperture_to_string(evwt, &aso, PRECISION_MODE_EIGHTH);
    printf("AP: %s\n", APERTURE_STRING_OUTPUT_STRING(aso));

    evwt.ev = (3+5)*8;
    evwt.tenths = 0;
    evwt = shutter_speed_given_aperture_iso_ev(9*8, 4*8, evwt);
    printf("VAL that should be equal to 0: %i\n", evwt.ev);
    assert(evwt.ev == 0);
    shutter_speed_to_string(evwt, &sso);
    printf("SHUT: %s\n", SHUTTER_STRING_OUTPUT_STRING(sso));

    return 0;
}

#endif
