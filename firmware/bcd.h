#ifndef BCD_H
#define BCD_H

uint8_t *bcd_add(uint8_t *digits1, uint8_t digits1_length,
                 uint8_t *digits2, uint8_t digits2_length);
uint8_t bcd_length_after_op(uint8_t *oldptr, uint8_t oldlength, uint8_t *newptr);
void bcd_to_string(uint8_t *digits, uint8_t length);
bool bcd_gt(uint8_t *digits1, uint8_t length1, uint8_t *digits2, uint8_t length2);
uint8_t *bcd_div_by(uint8_t *digits, uint8_t length, uint8_t by)

#endif

