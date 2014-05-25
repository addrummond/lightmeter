#include <menu_strings.h>

void menu_string_decode_(bool get_long, const uint8_t *menu_string, uint8_t *dest)
{
    uint16_t i = 0;
    bool in_longonly = false;
    for (;; ++i) {
        uint8_t bytei = i*6/8;
        uint8_t bitoff = i*6%8;

        uint8_t c = menu_string[bytei] >> bitoff;
        if (bitoff > 2)
            c |= menu_string[bytei+1] & (uint8_t)(0xFF >> (8 - (6 - bitoff)));

        if (c == 0) {
            dest[i] = 0;
            break;
        }
        else if (c == MENU_STRING_SPECIAL_LONGONLY) {
            in_longonly = !in_longonly;
        }
        else {
            if (in_longoly && !get_long)
                continue;

            dest[i] = c+1;
        }
    }
}
