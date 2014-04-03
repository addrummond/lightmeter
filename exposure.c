// Apertures are represented as unsigned 8-bit quantities
// from 1.4 to 32 in 1/8-stop steps.
//
// Shutter speeds are represented as unsigned 8-bit quantities
// from 1 minute to 1/16000 in 1/8-stop steps. Some key points
// on the scale are defined in exposure.h

#include <stdint.h>
#include <stdbool.h>
#include <exposure.h>
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
    uint16_t bytei = (uint16_t)speed;
    bytei = (bytei << 1) + (bytei >> 1);

    uint8_t shift = (speed & 1) << 2;

    uint8_t i, j, nibble;
    uint8_t previous = 0;
    bool already_got_slash = false;
    for (i = 0, j = 0; i < 5; ++j, ++i, shift ^= 4) {
        uint8_t nibble = (SHUTTER_SPEEDS[bytei] >> shift) & 0xF;
        if (nibble == 0)
            break;

        uint8_t c = SHUTTER_SPEEDS_BITMAP[nibble];
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
    uint8_t b = APERTURES[aperture >> 1];
    aso->chars[0] = APERTURES_BITMAP[b & 0xF];

    uint8_t high = (b >> 4) & 0xF;
    uint8_t c = APERTURES_BITMAP[high];
    uint8_t last = 1;

    if (high != 0)
        ++last;

    if (aperture <= AP_F9_5+1) {
        aso->chars[1] = '.';
        aso->chars[2] = c;
        ++last;
    }
    else {
        aso->chars[1] = c;
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

void iso_to_string(uint8_t iso, iso_string_output_t *out)
{
    // Repeatedly double via base 10 add with carry.
    // For ISOs <= 24(=25) we start from 6.
    // For larger ISOs we start from 100.
    // Special case at the end to convert 24 -> 25;
    //
    // TODO: This doesn't yet handle 1/8, 1/4 or 1/2 stop ISOs.
    // This is a tricky problem since there's no real convention
    // for how to number these. Maybe just add a special case
    // for ISO 125, since films hardly ever use any other non-full-stop
    // ISO ratings?

    // Init all to 0.
    uint8_t i;
    for (i = 0; i < sizeof(out->chars); ++i)
        out->chars[i] = 0;

    uint8_t end = sizeof(out->chars) - 2;
    uint8_t start = end;
    uint8_t times_to_double;

    if (iso <= 16/*ISO 24*/) {
        out->chars[end] = 6;
    }
    else if (iso < 64/*ISO 1600*/) {
        out->chars[end] = 0;
        out->chars[--start] = 5;
        iso -= 16;
    }
    else {
        out->chars[end] = 0;
        out->chars[--start] = 0;
        out->chars[--start] = 6;
        out->chars[--start] = 1;
        iso -= 50;
    }
    times_to_double = iso >> 3;

    uint8_t carry;
    for (uint8_t c = 0; c < times_to_double; ++c) {
        i = 0;
        carry = 0;

        for (i = end; i >= start; --i) {
            uint8_t doubled = (out->chars[i] << 1) + carry;
            if (doubled > 9) {
                out->chars[i] = doubled - 10;
                carry = 1;
                if (i == start)
                    --start;
            }
            else {
                carry = 0;
                out->chars[i] = doubled;
            }
        }
    }

    if (carry == 1) {
        out->chars[--start] = 1;
    }

    // Add base ASCII code for digits.
    for (i = start; i <= end; ++i) {
        out->chars[i] += 48;
    }
    // All chars were initialized to zero so it's already null-terminated.

    // 24 -> 25
    if (out->chars[end] == '4' && out->chars[end-1] == '2')
        out->chars[end] = '5';

    out->offset = start;
}

// We represent ISO in 1/8 stops, with 0 as ISO 6.
uint8_t aperture_given_shutter_speed_iso_ev(uint8_t speed_, uint8_t iso_, uint8_t ev_)
{
    // We know that at for EV=3, ISO = 6, speed = 1minute, aperture = 22.
    // Thus for EV = -5, ISO = 6, speed = 1minute, aperture = 22 - 8stops.

    int16_t the_aperture = (int16_t)AP_MAX + 8;
    int16_t the_speed = SS_1M;
    int16_t the_ev = 4*8; // -1 EV. +4 from base because we're on ISO 6 but EVs are given w.r.t. ISO 100.
    int16_t the_iso = 0;

    int16_t given_speed = (int16_t)speed_;
    int16_t given_iso = (int16_t)iso_;
    int16_t given_ev = (int16_t)ev_ + 4;

    int16_t isodiff = given_iso - the_iso; // Will always be positive or 0
    the_aperture -= isodiff;
    the_ev += isodiff;

    int16_t shutdiff = the_speed - given_speed; // Will always be positive.
    the_ev += shutdiff;

    int16_t evdiff = given_ev - the_ev;
    the_aperture -= evdiff;

    if (the_aperture > (int16_t)AP_MAX)
        return AP_MAX;
    else if (the_aperture < (int16_t)AP_MIN)
        return AP_MIN;
    return the_aperture;
}

#ifdef TEST

int main()
{
    shutter_string_output_t eso;
    uint8_t s;
    for (s = SS_MIN; s <= SS_MAX; ++s) {
        shutter_speed_to_string(s, &eso);
        printf("SS: %s\n", SHUTTER_STRING_OUTPUT_STRING(eso));
    }

    printf("\n");

    uint8_t a;
    aperture_string_output_t aso;
    for (a = AP_MIN; a < AP_MAX; ++a) {
        aperture_to_string(a, &aso);
        printf("A:  %s\n", APERTURE_STRING_OUTPUT_STRING(aso));
    }

    printf("\nExposures at ISO 100:\n");

    iso_string_output_t iso;
    for (uint8_t i = 0; i < 15*8; i += 8) {
        iso_to_string(i /* ISO 100 */, &iso);
        printf("ISO: %s\n", ISO_STRING_OUTPUT_STRING(iso));
    }
}

#endif
