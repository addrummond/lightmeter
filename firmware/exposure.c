// Apertures are represented as unsigned 8-bit quantities
// from 1.4 to 32 in 1/8-stop steps.
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
#include <divmulutils.h>
#ifdef TEST
#include <stdio.h>
#endif

extern uint8_t SHUTTER_SPEEDS[];
extern uint8_t SHUTTER_SPEEDS_BITMAP[];
extern uint8_t APERTURES[];
extern uint8_t APERTURES_BITMAP[];

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

    ++j;
    eso->length = j;
}

void aperture_to_string(uint8_t aperture, aperture_string_output_t *aso)
{
    if (aperture > AP_MAX)
        aperture = AP_MAX;

    uint8_t b = pgm_read_byte(&APERTURES[aperture >> 1]);
    aso->chars[0] = pgm_read_byte(&APERTURES_BITMAP[b & 0xF]);

    uint8_t high = (b >> 4) & 0xF;
    uint8_t c = pgm_read_byte(&APERTURES_BITMAP[high]);
    uint8_t last = 1;

    if (high != 0)
        ++last;

    if (high) {
        if (aperture <= AP_F9_5+1) {
            aso->chars[1] = '.';
            aso->chars[2] = c;
            ++last;
        }
        else {
            aso->chars[1] = c;
        }
    }

    if (aperture & 1) {
        aso->chars[last++] = '+';
        aso->chars[last++] = '1';
        aso->chars[last++] = '/';
        aso->chars[last++] = '8';
        aso->chars[last++] = 's';
        aso->chars[last++] = 't';
        aso->chars[last++] = 'p';
    }

    aso->chars[last] = '\0';

    ++last;
    aso->length = last;
}

// Convert a BCD ISO number into the closest equivalent 8-bit representation.
// In the 8-bit representation, ISOs start from 6 and go up in steps of 1/8 stop.
static const uint8_t BCD_50[] = { 5, 0 };
static const uint8_t BCD_6[] = { 6 };
uint8_t iso_bcd_to_stops(uint8_t *digits, uint8_t length)
{
    assert(length <= ISO_DECIMAL_MAX_DIGITS);

    uint8_t tdigits_[length];
    uint8_t *tdigits = tdigits_;
    uint8_t i;
    for (i = 0; i < length; ++i)
        tdigits_[i] = digits[i];

    uint8_t *r = tdigits;
    uint8_t iso8bit;
    for (iso8bit = 0; bcd_gteq(r, length, BCD_6, sizeof(BCD_6)); ++iso8bit) {
        // ISO 26 -> 25
        if (length == 2 && r[0] == 2 && r[1] == 5)
            r[1] = 6;
        // ISO 12500 -> 12800
        if (length == 6 && r[0] == '1' && r[1] == '2' && r[2] == '5' && r[3] == '0' && r[4] == '0' && r[5] == '0')
            r[2] = 8;

        r = bcd_div_by_lt10(r, length, 2);
        length = bcd_length_after_op(tdigits, length, r);
        tdigits = r;
    }

    return iso8bit-1;
}

// We represent ISO in 1/8 stops, with 0 as ISO 6.
uint8_t aperture_given_shutter_speed_iso_ev(uint8_t speed_, uint8_t iso_, uint8_t ev_)
{
    // We know that for EV=3, ISO = 100, speed = 1minute, aperture = 22.

    if (speed_ > SS_MAX)
        speed_ = SS_MAX;
    if (iso_ > ISO_MAX)
        iso_ = ISO_MAX;
    if (ev_ > EV_MAX)
        ev_ = EV_MAX;

    int16_t the_aperture = 9*8; // F22
    int16_t the_speed = SS_1M;
    int16_t the_ev = ((3+5)*8); // 3 EV
    int16_t the_iso = 3*8;      // 100 ISO

    int16_t given_speed = (int16_t)speed_;
    int16_t given_iso = (int16_t)iso_;
    int16_t given_ev = (int16_t)ev_;

    int16_t isodiff = given_iso - the_iso; // Will always be positive or 0
    the_aperture += isodiff;
    the_ev += isodiff;

    int16_t shutdiff = given_speed - the_speed; // Will always be positive.
    the_ev += shutdiff;

    int16_t evdiff = given_ev - the_ev;
    the_aperture += evdiff;

    if (the_aperture > (int16_t)AP_MAX)
        return AP_MAX;
    else if (the_aperture < (int16_t)AP_MIN)
        return AP_MIN;
    return the_aperture;
}

#ifdef TEST

int main()
{
#if 0
    shutter_string_output_t sso;
    uint8_t s;
    for (s = SS_MIN; s <= SS_MAX; ++s) {
        shutter_speed_to_string(s, &sso);
        printf("SS: %s\n", SHUTTER_STRING_OUTPUT_STRING(sso));
    }

    printf("\n");

    uint8_t a;
    aperture_string_output_t aso;
    for (a = AP_MIN; a <= AP_MAX; ++a) {
        aperture_to_string(a, &aso);
        printf("A:  %s\n", APERTURE_STRING_OUTPUT_STRING(aso));
    }

    printf("\nExposures at ISO 100:\n");

    iso_string_output_t iso;
    for (uint8_t i = 0; i < 15*8; i += 8) {
        iso_to_string(i /* ISO 100 */, &iso);
        printf("ISO: %s [%i]\n", ISO_STRING_OUTPUT_STRING(iso), i);
    }

    printf("\nTesting aperture_given_shutter_speed_iso_ev\n");
    uint8_t is, ss, ev, ap;
    for (is = ISO_MIN; is <= ISO_MAX; ++is) {
        for (ss = SS_MIN; ss <= SS_MAX; ++ss) {
             for (ev = 40; ev <= EV_MAX; ++ev) {
                ap = aperture_given_shutter_speed_iso_ev(ss, is, ev);
                shutter_speed_to_string(ss, &sso);
                aperture_to_string(ap, &aso);
                iso_to_string(is, &iso);
                printf("ISO %s,  %s  %s (EV = %.2f)   [%i, %i, %i : %i]\n",
                       ISO_STRING_OUTPUT_STRING(iso),
                       SHUTTER_STRING_OUTPUT_STRING(sso),
                       APERTURE_STRING_OUTPUT_STRING(aso),
                       ((float)(((int16_t)(ev))-(5*8)))/8.0,
                       is, ss, ev, ap);
             }
        }
    }
#endif

    static uint8_t isobcds[] = {
        7,   1, 6, 0, 0, 0, 0, 0,  0,
        6,   0, 8, 0, 0, 0, 0, 0,  0,
        6,   0, 4, 0, 0, 0, 0, 0,  0,
        6,   0, 2, 0, 0, 0, 0, 0,  0,
        6,   0, 1, 0, 0, 0, 0, 0,  0,
        5,   0, 0, 5, 0, 0, 0, 0,  0,
        5,   0, 0, 2, 5, 0, 0, 0,  0,
        5,   0, 0, 1, 2, 5, 0, 0,  0,
        4,   0, 0, 0, 6, 4, 0, 0,  0,
        4,   0, 0, 0, 3, 2, 0, 0,  0,
        4,   0, 0, 0, 1, 6, 0, 0,  0,
        3,   0, 0, 0, 0, 8, 0, 0,  0,
        3,   0, 0 ,0 ,0, 4, 0, 0,  0,
        3,   0, 0, 0, 0, 2, 0, 0,  0,
        3,   0, 0, 0, 0, 1, 0, 0,  0,
        2,   0, 0, 0, 0, 0, 5, 0,  0,
        2,   0, 0, 0, 0, 0, 2, 5,  0,
        2,   0, 0, 0, 0, 0, 1, 2,  0,
        1,   0, 0, 0, 0, 0, 0, 6,  0 
    };

    uint8_t i;
    for (i = 0; i < 9*19; i += 9) {
        uint8_t length = isobcds[i];
        uint8_t offset = i+8-length;
        uint8_t *isodigits = isobcds+offset;

        uint8_t stops = iso_bcd_to_stops(isodigits, length);
        bcd_to_string(isodigits, length);

        printf("ISO %s = %i stops from ISO 6\n", isodigits, stops);
    }

    return 0;
}

#endif
