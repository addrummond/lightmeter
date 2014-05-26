#ifdef TEST
#include <stdio.h>
#endif
#include <menus/menu_strings.h>

// Writes CHAR_12PX_*_CODE values into dest. (+1 because we reserve 0 as
// terminator).
uint8_t menu_string_decode_(bool get_long, const uint8_t *menu_string, uint8_t *dest)
{
    uint16_t i;
    uint8_t desti;
    bool in_longonly = false;
    uint8_t index_count = 0;
    uint8_t index1;
    uint8_t index2;
    bool big_index;
    for (i = 0, desti = 0;; ++i) {
        uint8_t bytei = i*6/8;
        uint8_t bitoff = i*6%8;

        uint8_t c = (pgm_read_byte(&menu_string[bytei]) >> bitoff) & 0b111111;
        if (bitoff > 2) {
            uint8_t bits = 8 - (bitoff - 2);
            uint8_t high = pgm_read_byte(&menu_string[bytei+1]) & (0xFF >> bits);
            high <<= 6-(bitoff-2);
            c |= high;
        }

        if (c == MENU_STRING_SPECIAL_DEFINCLUDE6) {
            big_index = false;
            index_count = 1;
            continue;
        }
        else if (c == MENU_STRING_SPECIAL_DEFINCLUDE12) {
            big_index = true;
            index_count = 2;
            continue;
        }
        else if (index_count == 2) {
            index2 = c;
            index_count = 1;
        }
        else if (index_count == 1) {
            index1 = c;
            index_count = 0;

            uint8_t index = index1;
            if (big_index)
                index |= index2 << 6;

            desti += menu_string_decode_(get_long, MENU_DEFINES[index], dest+desti) - 1;
        }
        else if (c == 0) {
            dest[desti++] = 0;
            break;
        }
        else if (c == MENU_STRING_SPECIAL_LONGONLY) {
            in_longonly = !in_longonly;
        }
        else {
            if (in_longonly && !get_long)
                continue;

            if (c == MENU_STRING_SPECIAL_SPACE)
                dest[desti++] = 255;
            else
                dest[desti++] = c;
        }
    }

    return desti;
}

#ifdef TEST

extern uint8_t MENU_DEFINE_0[];
int main()
{
    static uint8_t out[50];

    //menu_string_decode_long(MENU_DEFINE_0, out);
    menu_string_decode_long(MENU_STRING_FULL_STOPS_PM_1_2_STOPS, out);

    uint8_t i;
    for (i = 0; out[i]; ++i) {
        printf("%i ", out[i]);
    }
    printf("\n");
}

#endif
