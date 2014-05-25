#ifndef MENU_STRINGS_H
#define MENU_STRINGS_H

#include <stdint.h>
#include <stdbool.h>
#include <menus/menu_strings_table.h>

uint8_t menu_string_decode_(bool get_long, const uint8_t *menu_string, uint8_t *dest);
#define menu_string_decode_short(x,y) menu_string_decode_(false, (x), (y))
#define menu_string_decode_long(x,y) menu_string_decode_(true, (x), (y))

#endif
