#ifndef BCD_H
#define BCD_H

#include <stdint.h>
#include <mybool.h>

uint8_t *bcd_add_(uint8_t *digits1, uint8_t digits1_length, uint8_t *digits2, uint8_t digits2_length, uint8_t xoradd);
#define bcd_add(a,b,c,d) bcd_add_((a), (b), (c), (d), 0)
#define bcd_sub(a,b,c,d) bcd_add_((a), (b), (c), (d), 0xFF)
uint8_t *bcd_mul(uint8_t *digits1, uint8_t length1, const uint8_t *digits2, uint8_t length2);
void bcd_to_string(uint8_t *digits, uint8_t length);
void string_to_bcd(uint8_t *digits, uint8_t length);
bool bcd_cmp(const uint8_t *digits1, uint8_t length1, const uint8_t *digits2, uint8_t length2, uint8_t which);
uint8_t *bcd_div_by_lt10(uint8_t *digits, uint8_t length, uint8_t by);
uint8_t *uint8_to_bcd(uint8_t n, uint8_t *digits, uint8_t length);

#define BCD_EXP10_PRECISION 3
uint8_t bcd_exp10(uint8_t *digits, uint8_t length);

#define bcd_length_after_op(oldptr, oldlength, newptr) (((oldptr) - (newptr)) + (oldlength))

#ifdef TEST
void debug_print_bcd(uint8_t *digits, uint8_t length);
#endif

#define bcd_eq(a,b,c,d) bcd_cmp(a,b,c,d,0)
#define bcd_lteq(a,b,c,d) bcd_cmp(a,b,c,d,1)
#define bcd_gteq(a,b,c,d) bcd_cmp(a,b,c,d,2)
#define bcd_lt(a,b,c,d) bcd_cmp(a,b,c,d,3)
#define bcd_gt(a,b,c,d) bcd_cmp(a,b,c,d,4)

#endif
