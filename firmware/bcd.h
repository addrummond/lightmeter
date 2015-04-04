#ifndef BCD_H
#define BCD_H

#include <stdint.h>
#include <stdbool.h>

unsigned bcd_to_string_fp(uint8_t *digits, uint_fast8_t length, uint8_t *dest, uint_fast8_t sigfigs, uint_fast8_t dps);
#define bcd_to_string(digits, length) bcd_to_string_fp((digits), (length), (digits), 255, 255)

#ifdef TEST
void debug_print_bcd(uint8_t *digits, uint_fast8_t length);
#endif

#endif
