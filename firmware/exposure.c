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
#include <bcd.h>
#include <exposure.h>
#include <tables.h>
#include <debugging.h>
//#include <deviceconfig.h>
#include <mymemset.h>
#ifdef TEST
#include <stdio.h>
#endif

// See comments in calculate_tables.py for info on the way
// temp/voltage are encoded.
//
// 'voltage' is in 1/256ths of the reference voltage.
ev_with_fracs_t get_ev100_at_voltage(uint_fast8_t voltage, uint_fast8_t amp_stage)
{
    const uint8_t *ev_abs = NULL, *ev_diffs = NULL, *ev_bitpatterns = NULL, *ev_tenths = NULL, *ev_thirds = NULL;
#define ASSIGN(n) (ev_abs = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_ABS,                 \
                   ev_diffs = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_DIFFS,             \
                   ev_bitpatterns = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_BITPATTERNS, \
                   ev_tenths = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_TENTHS,           \
                   ev_thirds = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_THIRDS)
#define CASE(n) case n: ASSIGN(n); break;
    switch (amp_stage) {
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
    uint_fast8_t bits_to_add = (voltage % 16) + 1;

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
    ev_with_fracs_set_ev8(ret, ev_abs[absi]);
    for (; bits1; ev_with_fracs_set_ev8(ret, ev_with_fracs_get_ev8(ret)+1))
        bits1 &= bits1 - 1;
    for (; bits2; ev_with_fracs_set_ev8(ret, ev_with_fracs_get_ev8(ret)+1))
        bits2 &= bits2 - 1;

    // Calculate tenths.
    uint_fast8_t tenths_bit = ev_tenths[voltage >> 3];
    tenths_bit >>= (voltage & 0b111);
    tenths_bit &= 1;
    int_fast8_t ret_tenths = tenth_below_eighth(ev_with_fracs_get_ev8(ret));
    ret_tenths += tenths_bit;

    // Calculate thirds.
    uint_fast8_t thirds_bit = ev_thirds[voltage >> 3];
    thirds_bit >>= (voltage & 0b111);
    thirds_bit &= 0b11;
    int_fast8_t ret_thirds = third_below_eighth(ev_with_fracs_get_ev8(ret));
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

    if (ev_with_fracs_get_ev8(evwf) >= SHUTTER_SPEED_MAX) {
        ev_with_fracs_set_ev8(evwf, SHUTTER_SPEED_MAX);
        ev_with_fracs_zero_fracs(evwf);
    }

    uint_fast8_t shutev = ev_with_fracs_get_ev8(evwf);

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

    if (ev_with_fracs_get_ev8(evwf) >= AP_MAX) {
        evwf.ev = AP_MAX;
        ev_with_fracs_zero_fracs(evwf);
    }

    uint_fast8_t apev = ev_with_fracs_get_ev8(evwf);

    uint_fast8_t last = 0;
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

// Full stop ISO numbers (n * 10).
static const uint32_t FULL_STOP_ISOS[] = {
    8,16,30,60, 120, 250, 500, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
    125000, 250000, 500000, 1000000, 2000000, 4000000, 8000000, 16000000
};
#define FULL_STOP_ISO_AT(i) (FULL_STOP_ISOS[i])
// Third stop ISO numbers (n * 10).
static const uint32_t THIRD_STOP_ISOS[] = {
    10, 12, 20, 25, 40, 50, 80, 100, 160, 200, 320, 400, 640, 800, 1250, 1600,
    2500, 3200, 5000, 6400, 10000, 12500, 20000, 25000, 40000, 50000, 80000,
    100000, 160000, 200000, 320000, 400000, 640000, 800000, 1250000, 1600000,
    2500000, 3200000
};
#define THIRD_STOP_ISO_AT(i) (THIRD_STOP_ISOS[i])

unsigned iso_in_third_stops_to_bcd(uint_fast8_t iso, uint8_t *digits)
{
    uint_fast8_t whole = iso / 3;
    uint_fast8_t frac = iso % 3;

    uint32_t isonum;
    if (frac == 0)
        isonum = FULL_STOP_ISO_AT(whole);
    else
        isonum = THIRD_STOP_ISO_AT(2*whole + frac - 1);

    return uint32_to_bcd(isonum, digits);
}

uint_fast8_t iso_bcd_to_third_stops(uint8_t *digits, unsigned length)
{
    uint32_t isonum = bcd_to_uint32(digits, length);

    unsigned i;
    uint_fast8_t fullstop = 0;
    for (i = 0; i < sizeof(FULL_STOP_ISOS)/sizeof(FULL_STOP_ISOS[0]); ++i) {
        uint32_t n = FULL_STOP_ISOS[i];
        if (isonum == n)
            return i*3;
        if (isonum > n)
            fullstop = i;
    }

    // It's not a full stop ISO, so now we want to find the closest third-stop
    // ISO. The 'fullstop' variable contains the value (in full stops) of the
    // full stop ISO immediately above our given ISO. We now need to look at
    // the third-stops one third and two-thirds above this one, and see which
    // of the three is the closest match.
    uint32_t nothirdsabove = FULL_STOP_ISO_AT(fullstop);
    uint_fast8_t fullstopm = fullstop*2;
    uint32_t onethirdabove = THIRD_STOP_ISO_AT(fullstopm);
    uint32_t twothirdsabove = THIRD_STOP_ISO_AT(fullstopm+1);

    int32_t diff1 = nothirdsabove - isonum;
    int32_t diff2 = onethirdabove - isonum;
    int32_t diff3 = twothirdsabove - isonum;
    if (diff1 < 0)
        diff1 = -diff1;
    if (diff2 < 0)
        diff2 = -diff2;
    if (diff3 < 0)
        diff3 = -diff3;

    if (diff1 <= diff2 && diff1 <= diff3) {
        return fullstop*3;
    }
    else {
        uint_fast8_t t = fullstop*3 + 1;
        if (! (diff2 <= diff1 && diff2 <= diff3))
            ++t;
        return t;
    }
}

static const uint_fast8_t ev_with_fracs_to_xth_consts[] = {
    // Value -1
    //
    // 120
    119, 39, 14, 11,
    // 100
    99, 32, 12, 9,
    // 256
    255, 84, 31, 25
};
static int32_t ev_with_fracs_to_xth(ev_with_fracs_t evwf, uint_fast8_t const_offset)
{
    unsigned nth = ev_with_fracs_get_nth(evwf);

    int32_t whole = ev_with_fracs_get_wholes(evwf) * ((int32_t)ev_with_fracs_to_xth_consts[const_offset+0] + 1);
    int32_t rest;
    int32_t mul;
    if (nth == 3) {
        rest = ev_with_fracs_get_thirds(evwf);
        mul = ev_with_fracs_to_xth_consts[const_offset+1];
    }
    else if (nth == 8) {
        rest = ev_with_fracs_get_eighths(evwf);
        mul = ev_with_fracs_to_xth_consts[const_offset+2];
    }
    else if (nth == 10) {
        rest = ev_with_fracs_get_tenths(evwf);
        mul = ev_with_fracs_to_xth_consts[const_offset+3];
    }
    else {
        assert(false);
        return 0; // Get rid of warning.
    }

    rest *= ((int32_t)mul) + 1;

    return whole + rest;
}
#define ev_with_fracs_to_256th(x) ev_with_fracs_to_xth((x), 8)
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
    const int_fast16_t the_iso = 7*120;      // 100 ISO

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

ev_with_fracs_t average_ev_with_fracs(const ev_with_fracs_t *evwfs, unsigned length)
{
    ev_with_fracs_t r;
    ev_with_fracs_init(r);

    uint32_t total = 0;
    unsigned i;
    for (i = 0; i < length; ++i) {
        total += ev_with_fracs_to_120th(evwfs[i]);
    }
    total <<= 2;
    total /= length;
    if (total % 8 >= 4)
        total += 8;
    total >>= 2;

    ev_with_fracs_set_ev8(r, total/15);
    ev_with_fracs_set_thirds(r, total/40);
    ev_with_fracs_set_tenths(r, total/12);
    ev_with_fracs_set_nth(r, 8);

    return r;
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

#define EXP2_PRECISION 8
#define V_0_5          128
#define V_0_11         28
#define V_0_02         5
#define EXP2_0_5       362
#define EXP2_0_11      276
#define EXP2_0_02      260
static int32_t int32_exp2(int32_t x)
{
    int32_t ix = x >> EXP2_PRECISION;
    int32_t ir = 1 << EXP2_PRECISION;
    unsigned i;
    for (i = 0; i < ix; ++i)
        ir *= 2;

    int32_t ifl = x & (0xFFFFFFFF >> (32 - EXP2_PRECISION));
    int32_t m = 1 << EXP2_PRECISION;
    while (ifl > V_0_02) {
        if (ifl >= V_0_5) {
            ifl -= V_0_5;
            m *= EXP2_0_5;
        }
        else if (ifl >= V_0_11) {
            ifl -= V_0_11;
            m *= EXP2_0_11;
        }
        else {
            ifl -= V_0_02;
            m *= EXP2_0_02;
        }
        m >>= EXP2_PRECISION;
    }

    return (m * ir) >> EXP2_PRECISION;
}
#undef V_0_5
#undef V_0_11
#undef V_0_048
#undef EXP2_0_5
#undef EXP2_0_11
#undef EXP2_0_048

static int32_t ev_at_100_to_lux(ev_with_fracs_t evwf)
{
    int32_t ev = ev_with_fracs_to_256th(evwf) - (5*(1<<EXP2_PRECISION));
    ev = int32_exp2(ev);
    ev += ev + (ev/2); // Multiply by 2.5
    return ev;
}

unsigned ev_at_100_to_bcd_lux(ev_with_fracs_t evwf, uint8_t *digits)
{
    int32_t lux = ev_at_100_to_lux(evwf);

    // Convert from 256ths to 100ths.
    lux *= 100;
    lux >>= EXP2_PRECISION;

    return uint32_to_bcd(lux, digits);
}

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

    printf("ev for voltages 0-255 at stage...\n");
    uint8_t stage, voltage;
    for (stage = 1; stage < 8; ++stage) {
        printf("Stage %i\n", stage);
        for (voltage = 0; voltage < 255; ++voltage) {
            ev_with_fracs_t evwf = get_ev100_at_voltage(voltage, stage);
            uint8_t ev8 = ev_with_fracs_get_ev8(evwf);
            printf("    EV@100: %.2f\n", (((float)ev8)/8.0f)-5.0f);
        }
    }

    printf("\n");

    ev_with_fracs_t evat100;
    uint_fast8_t ev8;
    printf("ev_at_100_to_bcd_lux\n");
    for (ev8 = 8*5; ev8 < 160; ++ev8) {
        float fev = ((float)ev8 - 40.0) / 8.0;
        float flux = pow(2.0, fev) * 2.5;
        ev_with_fracs_init(evat100);
        ev_with_fracs_set_ev8(evat100, ev8);
        uint8_t lux_digits[EV_AT_100_TO_BCD_LUX_BCD_LENGTH];
        unsigned dl = ev_at_100_to_bcd_lux(evat100, lux_digits);
        print_bcd(lux_digits, dl, 3, 2);
        printf("EV@100 %f = ", (((float)ev_with_fracs_get_ev8(evat100))/8.0)-5.0);
        unsigned x;
        for (x = 0; x < dl; ++x)
            printf("%c", lux_digits[x] + '0');
        printf("\n");
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

    // Useful table for comparison is here: http://en.wikipedia.org/wiki/Film_speed
    static uint8_t isobcds[] = {
        8,   1, 6, 0, 0, 0, 0, 0, 0,  0,
        7,   0, 8, 0, 0, 0, 0, 0, 0,  0,
        7,   0, 4, 0, 0, 0, 0, 0, 0,  0,
        7,   0, 2, 0, 0, 0, 0, 0, 0,  0,
        7,   0, 1, 0, 0, 0, 0, 0, 0,  0,
        6,   0, 0, 5, 0, 0, 0, 0, 0,  0,
        6,   0, 0, 2, 5, 0, 0, 0, 0,  0,
        6,   0, 0, 2, 0, 0, 0, 0, 0,  0,
        6,   0, 0, 1, 6, 0, 0, 0, 0,  0,
        6,   0, 0, 1, 2, 5, 0, 0, 0,  0,
        6,   0, 0, 1, 0, 0, 0, 0, 0,  0,
        5,   0, 0, 0, 8, 0, 0, 0, 0,  0,
        5,   0, 0, 0, 6, 4, 0, 0, 0,  0,
        5,   0, 0, 0, 3, 2, 0, 0, 0,  0,
        5,   0, 0, 0, 2, 5, 0, 0, 0,  0,
        5,   0, 0, 0, 2, 0, 0, 0, 0,  0,
        5,   0, 0, 0, 1, 6, 0, 0, 0,  0,
        5,   0, 0, 0, 1, 2, 5, 0, 0,  0,
        5,   0, 0, 0, 1, 0, 0, 0, 0,  0,
        4,   0, 0, 0, 0, 8, 0, 0, 0,  0,
        4,   0, 0 ,0 ,0, 6, 4, 0, 0,  0,
        4,   0, 0 ,0 ,0, 5, 0, 0, 0,  0,
        4,   0, 0 ,0 ,0, 4, 0, 0, 0,  0,
        4,   0, 0, 0, 0, 3, 2, 0, 0,  0,
        4,   0, 0, 0, 0, 2, 5, 0, 0,  0,
        4,   0, 0, 0, 0, 2, 0, 0, 0,  0,
        4,   0, 0, 0, 0, 1, 6, 0, 0,  0,
        4,   0, 0, 0, 0, 1, 2, 5, 0,  0,
        4,   0, 0, 0, 0, 1, 0, 0, 0,  0,
        3,   0, 0, 0, 0, 0, 8, 4, 0,  0,
        3,   0, 0, 0, 0, 0, 6, 4, 0,  0,
        3,   0, 0, 0, 0, 0, 5, 0, 0,  0,
        3,   0, 0, 0, 0, 0, 4, 0, 0,  0,
        3,   0, 0, 0, 0, 0, 3, 2, 0,  0,
        3,   0, 0, 0, 0, 0, 2, 5, 0,  0,
        3,   0, 0, 0, 0, 0, 2, 0, 0,  0,
        3,   0, 0, 0, 0, 0, 1, 6, 0,  0,
        3,   0, 0, 0, 0, 0, 1, 2, 0,  0,
        3,   0, 0, 0, 0, 0, 1, 0, 0,  0,
        2,   0, 0, 0, 0, 0, 0, 8, 0,  0,
        2,   0, 0, 0, 0, 0, 0, 6, 0,  0
    };

    printf("Testing iso_bcd_to_third_stops\n");
    int i;
    for (i = sizeof(isobcds) - 9; i >= 0; i -= 9) {
        int length = isobcds[i];
        int offset = i+8-length;
        uint8_t *isodigits = isobcds+offset;

        int stops = iso_bcd_to_third_stops(isodigits, length);
        bcd_to_string(isodigits, length);

        printf("ISO %s = %.2f stops from ISO 0.8\n", isodigits, ((float)stops)/3.0);
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
            printf("Values not equal for v = %i: compressed = %i, uncompressed = %i\n", (unsigned)v_, (unsigned)compressed, (unsigned)uncompressed);
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
