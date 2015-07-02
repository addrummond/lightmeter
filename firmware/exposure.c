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
// voltage and ev are encoded.
//
// 'voltage' is in 1/256ths of the reference voltage.
ev_with_fracs_t get_ev100_at_voltage(uint_fast8_t voltage, uint_fast8_t amp_stage)
{
    const uint8_t *ev_abs = NULL, *ev_diffs = NULL, *ev_eighths = NULL, *ev_thirds = NULL;
#define ASSIGN(n) (ev_abs = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_ABS,                 \
                   ev_diffs = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_DIFFS,             \
                   ev_eighths = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_EIGHTHS,         \
                   ev_thirds = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_THIRDS)
#define CASE(n) case n: ASSIGN(n); break;
    switch (amp_stage) {
FOR_EACH_AMP_STAGE(CASE)
    }
#undef ASSIGN
#undef CASE

    if (voltage < VOLTAGE_TO_EV_ABS_OFFSET)
        voltage = 0;
    else
        voltage -= VOLTAGE_TO_EV_ABS_OFFSET;

    uint_fast8_t absi = voltage / 16;
    uint_fast8_t bits_to_add = (voltage % 16)+1;

    uint_fast8_t bits1 = ev_diffs[absi*2];
    uint_fast8_t bits2 = ev_diffs[absi*2+1];

    // See http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
    int32_t total_tenths = ((int32_t)(ev_abs[absi])) - (5*10);
    if (bits_to_add <= 8) {
         bits1 &= (0xFF >> (8 - bits_to_add));
         bits2 = 0;
    }
    else {
        bits2 &= (0xFF >> (16 - bits_to_add));
    }

    for (; bits1; ++total_tenths)
        bits1 &= bits1 - 1;
    for (; bits2; ++total_tenths)
        bits2 &= bits2 - 1;

    int32_t tenths = total_tenths % 10;
    int32_t wholes = total_tenths / 10;

    // Calculate eighths.
    uint_fast8_t eighths_bit = ev_eighths[voltage/8];
    eighths_bit >>= 7 - (voltage % 8);
    eighths_bit &= 1;
    int32_t eighths = eighth_below_tenth(tenths);
    eighths += eighths_bit;

    // Calculate thirds.
    uint_fast8_t thirds_bit = ev_thirds[voltage/8];
    thirds_bit >>= 7 - (voltage % 8);
    thirds_bit &= 1;
    int32_t thirds = third_below_tenth(tenths);
    thirds += thirds_bit;

    ev_with_fracs_t avg = (ev_with_fracs_t)((wholes*120*3 + (thirds*(120/3)) + (eighths*(120/8)) + (tenths*(120/10)))/3);
    //printf("TRYING %i 10=%i, 8=%i, 3=%i [%i, %i, %i]\n", avg, ev_with_fracs_get_nearest_tenths(avg), ev_with_fracs_get_nearest_eighths(avg), ev_with_fracs_get_nearest_thirds(avg), tenths, eighths, thirds);

    // The average gives us our starting point. Now we tweak the value to make
    // sure we get back the right number of tenths, eighths and thirds.
    unsigned i;
    int32_t e1 = avg+1, e2 = avg-1;
    int32_t lowest = 1000000, highest = -1000000;
    for (i = 0; i < 120/4; ++i, ++e1, --e2) {
        int32_t ctenths1  = ev_with_fracs_get_nearest_tenths((ev_with_fracs_t)e1);
        int32_t ceighths1 = ev_with_fracs_get_nearest_eighths((ev_with_fracs_t)e1);
        int32_t cthirds1  = ev_with_fracs_get_nearest_thirds((ev_with_fracs_t)e1);
        if (ctenths1 == tenths && ceighths1 == eighths && cthirds1 == thirds && ev_with_fracs_get_wholes((ev_with_fracs_t)e1) == wholes) {
            if (e1 > highest)
                highest = e1;
            if (e1 < lowest)
                lowest = e1;
        }

        int32_t ctenths2  = ev_with_fracs_get_nearest_tenths((ev_with_fracs_t)e2);
        int32_t ceighths2 = ev_with_fracs_get_nearest_eighths((ev_with_fracs_t)e2);
        int32_t cthirds2  = ev_with_fracs_get_nearest_thirds((ev_with_fracs_t)e2);
        if (ctenths2 == tenths && ceighths2 == eighths && cthirds2 == thirds && ev_with_fracs_get_wholes((ev_with_fracs_t)e2) == wholes) {
            if (e2 > highest)
                highest = e2;
            if (e2 < lowest)
                lowest = e2;
        }
    }

    assert(lowest != 1000000 && highest != -1000000);

    return (ev_with_fracs_t)((lowest + highest)/2);
}

#define pm_8_4_2(pm) (((pm) == PRECISION_MODE_EIGHTH) || ((pm) == PRECISION_MODE_QUARTER) || ((pm) == PRECISION_MODE_HALF))

void shutter_speed_to_string(ev_with_fracs_t evwf, shutter_string_output_t *sso, precision_mode_t precision_mode)
{
    if (ev_with_fracs_get_wholes(evwf) >= SHUTTER_SPEED_MAX_WHOLE_STOPS)
        ev_with_fracs_init_from_wholes(evwf, SHUTTER_SPEED_MAX_WHOLE_STOPS);

    uint_fast8_t shutwholeev = ev_with_fracs_get_wholes(evwf);

    uint8_t *schars;
    if (precision_mode == PRECISION_MODE_THIRD) {
        schars = SHUTTER_SPEEDS_THIRD + (shutwholeev*3*2);
        uint_fast8_t thirds = ev_with_fracs_get_nearest_thirds(evwf);
        schars += thirds*2;
    }
    else if (precision_mode == PRECISION_MODE_EIGHTH) {
        schars = SHUTTER_SPEEDS_EIGHTH + (shutwholeev*8*2);
        uint_fast8_t eighths = ev_with_fracs_get_nearest_eighths(evwf);
        schars += eighths*2;
    }
    else if (precision_mode == PRECISION_MODE_QUARTER) {
        // TODO
        schars = 0;
    }
    else if (precision_mode == PRECISION_MODE_HALF) {
        // TODO
        schars = 0;
    }
    else { //if (precision_mode == PRECISION_MODE_TENTH) {
        schars = SHUTTER_SPEEDS_TENTH + (shutwholeev*10*2);
        uint_fast8_t tenths = ev_with_fracs_get_nearest_tenths(evwf);
        schars += tenths*2;
    }

    uint_fast8_t last = 0;
    if ((shutwholeev > 6) || (shutwholeev == 6 && !ev_with_fracs_is_whole(evwf))) {
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

    if (shutwholeev < 6 || (shutwholeev == 6 && ev_with_fracs_is_whole(evwf)))
        sso->chars[last++] = 'S';

    sso->chars[last] = '\0';
    sso->length = last;
}

void aperture_to_string(ev_with_fracs_t evwf, aperture_string_output_t *aso, precision_mode_t precision_mode)
{
    if (ev_with_fracs_get_wholes(evwf) >= AP_MAX_WHOLE_STOPS)
        ev_with_fracs_init_from_wholes(evwf, AP_MAX_WHOLE_STOPS);

    uint_fast8_t apwholeev = ev_with_fracs_get_wholes(evwf);

    uint_fast8_t last = 0;
    if (precision_mode == PRECISION_MODE_THIRD) {
        uint_fast8_t thirds = ev_with_fracs_get_nearest_thirds(evwf);
        uint_fast8_t b = APERTURES_THIRD[(apwholeev*3) + thirds];
        last = 0;
        aso->chars[last++] = '0' + (b & 0xF);
        if ((apwholeev < 6) || (apwholeev == 6 && thirds <= 1))
            aso->chars[last++] = '.';
        aso->chars[last++] = '0' + (b >> 4);
    }
    else if (precision_mode == PRECISION_MODE_TENTH || pm_8_4_2(precision_mode)) {
        // i is index in nibbles.
        unsigned i;
        uint8_t *a;
        if (pm_8_4_2(precision_mode)) {
            uint_fast8_t eighths = ev_with_fracs_get_nearest_eighths(evwf);
            i = (apwholeev*8*3) + (eighths*3);
            a = APERTURES_EIGHTH;
        }
        else { //if (precision_mode == PRECISION_MODE_TENTH) {
            uint_fast8_t tenths = ev_with_fracs_get_nearest_tenths(evwf);
            i = (apwholeev*10*3) + (tenths*3);
            a = APERTURES_TENTH;
        }

        unsigned bi = i/2; // Calculate index of first byte in array that we're interested in.
        uint_fast8_t d1v, d2v, d3v;
        if (i % 2 == 0) {
            d1v = a[bi] >> 4;
            d2v = a[bi] & 0xF;
            d3v = a[bi+1] >> 4;
        }
        else {
            d1v = a[bi] & 0xF;
            d2v = a[bi+1] >> 4;
            d3v = a[bi+1] & 0xF;
        }

        uint_fast8_t d1 = '0' + d1v, d2 = '0' + d2v, d3 = '0' + d3v;

        last = 0;
        aso->chars[last++] = d1;
        if ((apwholeev < 1) ||
            (precision_mode == PRECISION_MODE_TENTH && (apwholeev < 6 || (apwholeev == 6 && ev_with_fracs_get_nearest_tenths(evwf) < 7))) ||
            (pm_8_4_2(precision_mode) && (apwholeev < 6 || (apwholeev == 6 && ev_with_fracs_get_nearest_eighths(evwf) < 6)))) {
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

    // If we're doing half or quarter-stop precision, go down to two sig figs.
    // Note that we can get away with not doing proper rounding calculations due
    // to the particular values in the table.
    if (precision_mode == PRECISION_MODE_HALF || precision_mode == PRECISION_MODE_QUARTER) {
        uint8_t lastdigitval = aso->chars[aso->length-1];
        if (aso->chars[aso->length-2] == '.') {
            if (lastdigitval >= '5')
                ++(aso->chars[aso->length-3]);
            aso->length -= 2;
        }
        else {
            if (lastdigitval >= '5')
                ++(aso->chars[aso->length-2]);
            aso->length -= 1;
        }
    }

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

    // For some reason the standard half-stop scale uses f3.3 rather than f3.4,
    // which is a rounding error. (Perhaps the result of calculating from f2.8
    // rather than from f2.8284... ?) Curiously, the same mistake is not repeated
    // in the quarter-stop scale listed on Wikipedia. Googling suggests that f3.3
    // is indeed used far more widely than f3.4, so we just change f3.4 to f3.3
    // when using half stop precision. Quarter stop apertures are used too rarely
    // for there to be any really standard behavior, so we give the correct value
    // in quarter stop mode.
    //
    // Exactly the same situation obtains for f13 (incorrect) vs f14. The incorrect
    // value is used in the half stop scale and the correct value is used in the
    // quarter stop scale.
    //
    if (precision_mode == PRECISION_MODE_HALF) {
        if (aso->chars[0] == '3' && aso->chars[1] == '.' && aso->chars[2] == '4')
            aso->chars[2] = '3';
        else if (aso->chars[0] == '1' && aso->chars[1] == '4')
            aso->chars[1] = '3';
    }

    // Add null terminator.
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

// This is called by the following macros defined in exposure.h:
//
//     aperture_given_shutter_speed_iso_ev(shutter_speed,iso,ev)
//     shutter_speed_given_aperture_iso_ev(aperture,iso,ev)
//     iso_given_aperture_shutter_speed(aperture, shutter_speed)
ev_with_fracs_t z_given_x_y_ev(ev_with_fracs_t given_x_, ev_with_fracs_t given_y_, ev_with_fracs_t evwf, uint_fast8_t x) // x=0: aperture, x=1: shutter_speed
{
    // Update: Currently, ev_with_fracs_t is implemented as an integer value
    // in 1/120 stop units, so the conversion below is trivial.

    // We use an internal represenation of values in 1/120 EV steps. This permits
    // exact division by 8, 10 and 3.

    // We know that for EV=3, ISO = 100, speed = 1minute, aperture = 22.
    const int32_t the_aperture = 9*120; // F22
    const int32_t the_speed = 0;        // 1 minute.
    const int32_t the_ev = (3)*120;     // 3 EV
    const int32_t the_iso = 7*120;      // 100 ISO

    int32_t given_x = ev_with_fracs_to_int32_120th(given_x_);
    int32_t given_y = ev_with_fracs_to_int32_120th(given_y_);
    int32_t given_ev = ev_with_fracs_to_int32_120th(evwf);

    int32_t r;
    int32_t min, max;
    if (x == 0) {
        int32_t shut_adjusted = the_ev + given_x - the_speed;

        int32_t evdiff = given_ev - shut_adjusted;
        r = the_aperture + evdiff;
        // Adjust for difference between reference ISO and actual ISO.
        r += given_y - the_iso;
        min = AP_MIN_WHOLE_STOPS*120, max = AP_MAX_WHOLE_STOPS*120;
    }
    else if (x == 1) {
        int32_t ap_adjusted = the_ev + given_x - the_aperture;
        int32_t evdiff = given_ev - ap_adjusted;
        r = the_speed + evdiff;
        // Adjust for difference between reference ISO and actual ISO.
        r += given_y - the_iso;
        min = SHUTTER_SPEED_MIN_WHOLE_STOPS*120, max = SHUTTER_SPEED_MAX_WHOLE_STOPS*120;
    }
    else { // x == 2
        int32_t iso_adjusted = the_ev + given_x - the_aperture;
        int32_t evdiff = given_ev - iso_adjusted;
        r = the_iso + evdiff;
        // Adjust for difference between reference shutter speed and actual shutter speed.
        r += given_y - the_speed;
        min = ISO_MIN_WHOLE_STOPS*120, max = ISO_MAX_WHOLE_STOPS*120;
    }

    if (r < min)
        r = min;
    else if (r > max)
        r = max;

    ev_with_fracs_init_from_120ths(evwf, r);
    return evwf;
}

ev_with_fracs_t average_ev_with_fracs(const ev_with_fracs_t *evwfs, unsigned length)
{
    int32_t total = 0;

    unsigned i;
    for (i = 0; i < length; ++i) {
        total += ev_with_fracs_to_int32_120th(evwfs[i]);
    }
    total <<= 2;
    total /=length;
    if (total % 8 >= 4)
        total += 8;
    total >>= 2;

    ev_with_fracs_t ret;
    ev_with_fracs_init_from_120ths(ret, total);

    return ret;
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
    int32_t ev = (int_fast16_t)log_base2(fp32);
    // Divide by 10 (because we're using units of 1/10 frames).
#if LOG2_PRECISION == 6
    ev -= 0b11010101; // 11.010101 = 3.322 = log2(10) with 6 precision bits.
#else
#error "Bad LOG2_PRECISION value"
#endif

    // Add magic number to get shutter speed EV (+ 7EV).
    ev += 7 * (1 << LOG2_PRECISION);

    if (ev > SHUTTER_SPEED_MAX_WHOLE_STOPS * (1 << LOG2_PRECISION))
        ev = SHUTTER_SPEED_MAX_WHOLE_STOPS * (1 << LOG2_PRECISION);
    else if (ev < SHUTTER_SPEED_MIN_WHOLE_STOPS * (1 << LOG2_PRECISION))
        ev = SHUTTER_SPEED_MIN_WHOLE_STOPS * (1 << LOG2_PRECISION);

    ev *= EV_WITH_FRACS_TH;
    int32_t rem = ev % (1 << LOG2_PRECISION);
    if (rem >= (1 << LOG2_PRECISION)/2)
        ev += (1 << LOG2_PRECISION)/2;
    ev /= (1 << LOG2_PRECISION);

    return (ev_with_fracs_t)ev;
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
    int32_t ev = ev_with_fracs_to_int32_256th(evwf);
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

static float evwf_to_float(ev_with_fracs_t evwf)
{
    return ((float)evwf)/120.0f;
}

int main()
{
    aperture_string_output_t aso;
    shutter_string_output_t sso;
    ev_with_fracs_t iso100;
    ev_with_fracs_init_from_wholes(iso100, 7);

    printf("ev for voltages 0-255 at stage...\n");
    uint8_t stage, voltage;
    for (stage = 1; stage < NUM_AMP_STAGES; ++stage) {
        printf("Stage %i\n", stage);
        for (voltage = VOLTAGE_TO_EV_ABS_OFFSET; voltage < 255; ++voltage) {
            ev_with_fracs_t evwf = get_ev100_at_voltage(voltage, stage);
            printf("    EV@100: %i -> %.1f (%.3f)\n", voltage, evwf_to_float(evwf), evwf_to_float(evwf));
        }
    }

    printf("\n");

    // TODO: Bugs in print_bcd; some kind of overflow issue for larger ev values.
    ev_with_fracs_t evat100;
    int32_t ev10;
    printf("ev_at_100_to_bcd_lux\n");
    for (ev10 = 0; ev10 < 10*10; ++ev10) {
        float fev = ((float)ev10)/10.0;
        float flux = pow(2.0, fev) * 2.5;
        ev_with_fracs_init_from_tenths(evat100, ev10);
        uint8_t lux_digits[EV_AT_100_TO_BCD_LUX_BCD_LENGTH];
        unsigned dl = ev_at_100_to_bcd_lux(evat100, lux_digits);
        print_bcd(lux_digits, dl, 3, 2);
        printf(" [%i] ", dl);
        printf(" EV@100 %f = ", (((float)ev_with_fracs_to_int32_120th(evat100))/120.0));
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
    for (s = SHUTTER_SPEED_MIN_WHOLE_STOPS*8; s <= SHUTTER_SPEED_MAX_WHOLE_STOPS*8; ++s) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init_from_eighths(evwf, s);
        shutter_speed_to_string(evwf, &sso, PRECISION_MODE_EIGHTH);
        printf("SS: %s (evw=%i+1/%i, ev*8 = %i, length = %i)\n", SHUTTER_STRING_OUTPUT_STRING(sso), s/8, s%8, s, sso.length);
    }


    printf("\n");

    printf("Shutter speeds in tenths:\n");
    for (s = SHUTTER_SPEED_MIN_WHOLE_STOPS*10; s <= SHUTTER_SPEED_MAX_WHOLE_STOPS*10; ++s) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init_from_tenths(evwf, s);
        shutter_speed_to_string(evwf, &sso, PRECISION_MODE_TENTH);
        printf("SS: %s (ev = %i.%i, length = %i)\n", SHUTTER_STRING_OUTPUT_STRING(sso), s/10, s%10, sso.length);
    }

    printf("\n");

    printf("Shutter speeds in thirds:\n");
    for (s = SHUTTER_SPEED_MIN_WHOLE_STOPS*3; s <= SHUTTER_SPEED_MAX_WHOLE_STOPS*3; ++s) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init_from_thirds(evwf, s);
        shutter_speed_to_string(evwf, &sso, PRECISION_MODE_THIRD);
        printf("SS: %s (ev = %i+%i/3, length = %i)\n", SHUTTER_STRING_OUTPUT_STRING(sso), s/3, s%3, sso.length);
    }

    printf("\n");

    printf("Apertures in halves:\n");
    uint_fast8_t a;
    for (a = AP_MIN_WHOLE_STOPS*2; a <= AP_MAX_WHOLE_STOPS*2; ++a) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init_from_halves(evwf, a);
        aperture_to_string(evwf, &aso, PRECISION_MODE_HALF);
        printf("A: %s (ev = %i+%i/2)\n", APERTURE_STRING_OUTPUT_STRING(aso), a/2, a%2);
    }

    printf("\n");

    printf("Apertures in thirds:\n");
    for (a = AP_MIN_WHOLE_STOPS*3; a <= AP_MAX_WHOLE_STOPS*3; ++a) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init_from_thirds(evwf, a);
        aperture_to_string(evwf, &aso, PRECISION_MODE_THIRD);
        printf("A: %s (ev = %i+%i/3)\n", APERTURE_STRING_OUTPUT_STRING(aso), a/3, a%3);
    }

    printf("\n");

    printf("Apertures in quarters:\n");
    for (a = AP_MIN_WHOLE_STOPS*4; a <= AP_MAX_WHOLE_STOPS*4; ++a) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init_from_quarters(evwf, a);
        aperture_to_string(evwf, &aso, PRECISION_MODE_QUARTER);
        printf("A: %s (ev = %i+%i/4)\n", APERTURE_STRING_OUTPUT_STRING(aso), a/4, a%4);
    }

    printf("\n");

    printf("Apertures in eighths:\n");
    for (a = AP_MIN_WHOLE_STOPS*8; a <= AP_MAX_WHOLE_STOPS*8; ++a) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init_from_eighths(evwf, a);
        aperture_to_string(evwf, &aso, PRECISION_MODE_EIGHTH);
        printf("A: %s (ev = %i+%i/8)\n", APERTURE_STRING_OUTPUT_STRING(aso), a/8, a%8);
    }

    printf("\n");

    printf("Apertures tenths:\n");
    for (a = AP_MIN_WHOLE_STOPS*10; a <= AP_MAX_WHOLE_STOPS*10; ++a) {
        ev_with_fracs_t evwf;
        ev_with_fracs_init_from_tenths(evwf, a);
        aperture_to_string(evwf, &aso, PRECISION_MODE_EIGHTH);
        printf("A: %s (ev = %i.%i)\n", APERTURE_STRING_OUTPUT_STRING(aso), a/10, a%10);
    }

    printf("\n");

    printf("\nTesting aperture_given_shutter_speed_iso_ev\n");
    uint_fast8_t is, ss, ev, ap;
    for (is = ISO_MIN_WHOLE_STOPS*3; is <= ISO_MAX_WHOLE_STOPS*3; ++is) {
        ss = 8*8;  // 1/60
        ev = 10*8; // 10 EV
        ev_with_fracs_t evwf;
        ev_with_fracs_init_from_eighths(evwf, ev);
        ev_with_fracs_t isoevwf;
        ev_with_fracs_init_from_thirds(isoevwf, is);
        //printf("ISO EV %i (%i,%i)*****\n", ev_with_fracs_get_ev8(isoevwf), ((is/3)*8) + eighths_from_thirds(is%3), is);
        //printf("ISO third %i *****\n", ev_with_fracs_get_thirds(isoevwf));
        ev_with_fracs_t ssevwf;
        ev_with_fracs_init_from_eighths(ssevwf, ss);
        evwf = aperture_given_shutter_speed_iso_ev(ssevwf, isoevwf, evwf);
        shutter_speed_to_string(ssevwf, &sso, PRECISION_MODE_EIGHTH);
        aperture_to_string(evwf, &aso, PRECISION_MODE_EIGHTH);
        printf("ISO %f stops from 6,  %s  %s (EV = %.2f)   [%i, %i, %i : %i]\n",
               ((float)is) / 3.0,
               SHUTTER_STRING_OUTPUT_STRING(sso),
               APERTURE_STRING_OUTPUT_STRING(aso),
               ((float)(((int_fast16_t)(ev))))/8.0,
               is, ss, ev, ap);
    }

    printf("\nTesting shutter_speed_given_aperture_iso_ev\n");
    for (is = ISO_MIN_WHOLE_STOPS*3; is <= ISO_MAX_WHOLE_STOPS*3; ++is) {
        ap = 6*8; // f8
        ev = 10*8; // 10 EV
        ev_with_fracs_t evwf;
        ev_with_fracs_init_from_eighths(evwf, ev);
        ev_with_fracs_t apevwf;
        ev_with_fracs_init_from_eighths(apevwf, ap);
        ev_with_fracs_t isoevwf;
        ev_with_fracs_init_from_thirds(isoevwf, is);
        evwf = shutter_speed_given_aperture_iso_ev(apevwf, isoevwf, evwf);
        shutter_speed_to_string(evwf, &sso, PRECISION_MODE_EIGHTH);
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
    for (i = sizeof(isobcds) - 10; i >= 0; i -= 10) {
        int length = isobcds[i];
        int offset = i+9-length;
        uint8_t *isodigits = isobcds+offset;

        int stops = iso_bcd_to_third_stops(isodigits, length);
        bcd_to_string(isodigits, length);

        printf("ISO %s/10 = %.2f stops from ISO 0.8\n", isodigits, ((float)stops)/3.0);
    }
}

#endif
