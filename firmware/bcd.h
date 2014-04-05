#ifndef BCD_H
#define BCD_H

uint8_t *bcd_add(uint8_t *digits1, uint8_t digits1_length,
                 uint8_t *digits2, uint8_t digits2_length);
uint8_t bcd_length_after_op(const uint8_t *oldptr, uint8_t oldlength, const uint8_t *newptr);
void bcd_to_string(uint8_t *digits, uint8_t length);
bool bcd_cmp(const uint8_t *digits1, uint8_t length1, const uint8_t *digits2, uint8_t length2, uint8_t which);
uint8_t *bcd_div_by_lt10(uint8_t *digits, uint8_t length, uint8_t by);

#define bcd_eq(a,b,c,d) bcd_cmp(a,b,c,d,0)
#define bcd_lteq(a,b,c,d) bcd_cmp(a,b,c,d,1)
#define bcd_gteq(a,b,c,d) bcd_cmp(a,b,c,d,2)
#define bcd_lt(a,b,c,d) bcd_cmp(a,b,c,d,3)
#define bcd_gt(a,b,c,d) bcd_cmp(a,b,c,d,4)

#endif

