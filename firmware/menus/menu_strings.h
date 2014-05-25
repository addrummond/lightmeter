#ifndef MENU_STRINGS_H
#define MENU_STRINGS_H

#include <stdint.h>
#include <menu_strings_table.h>

void menu_string_decode(bool get_long, const uint8_t *menu_string, uint8_t *dest);

#endif
