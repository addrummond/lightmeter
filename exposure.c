// Apertures are represented as unsigned 8-bit quantities
// from 1.4 to 32 in 1/8-stop steps.
//
// Shutter speeds are represented as unsigned 8-bit quantities
// from 1 minute to 1/16000 in 1/8-stop steps. Some key points
// on the scale are defined in exposure.h

#include <stdint.h>
#include <exposure.h>
#include <divmulutils.h>
#ifdef TEST
#include <stdio.h>
#endif

extern uint8_t SHUTTER_SPEEDS[];
extern uint8_t SHUTTER_SPEEDS_BITMAP[];
extern uint8_t APERTURES[];
extern uint8_t APERTURES_BITMAP[];

void shutter_speed_to_string(uint8_t speed, exposure_string_output_t *eso)
{
    uint16_t bytei = (uint16_t)speed;
    bytei = (bytei << 1) + (bytei >> 1);

    uint8_t shift = (speed & 1) << 2;

    uint8_t i, j, nibble;
    for (i = 0, j = 0; i < 5; ++j, ++i, shift ^= 4) {
        uint8_t nibble = (SHUTTER_SPEEDS[bytei] >> shift) & 0xF;
        if (nibble == 0) {
            break;
        }
        uint8_t c = SHUTTER_SPEEDS_BITMAP[nibble];
        if (c == '+')
            eso->chars[j++] = 'S';
        eso->chars[j] = c;

        bytei += ((shift & 4) >> 2);
    }

    // Add any required trailing zeros. TODO: could probably get rid of the conditionals.
    if (speed >= SS_1000TH)
        eso->chars[j++] = '0';
    if (speed >= SS_16000TH)
        eso->chars[j++] = '0';

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

#ifdef TEST

int main()
{
    exposure_string_output_t eso;
    uint8_t s;
    for (s = SS_MIN; s <= SS_MAX; ++s) {
        shutter_speed_to_string(s, &eso);
        printf("SS: %s\n", eso.chars);
    }

    printf("\n");

    uint8_t a;
    aperture_string_output_t aso;
    for (a = AP_MIN; a < AP_MAX; ++a) {
        aperture_to_string(a, &aso);
        printf("A:  %s\n", aso.chars);
    }
}

#endif
