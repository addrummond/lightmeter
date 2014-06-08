#ifndef SHIFTREGISTER_H
#define SHIFTREGISTER_H

void setup_shift_register(void);
void set_shift_register_out(void);

extern volatile uint8_t shift_register_out__;

#define or_shift_register_bits(n) (shift_register_out__ |= (n))
#define and_shift_register_bits(n) (shift_register_out__ &= (n))
#define set_shift_register_bits(n) (shift_register_out__ = (n))

#endif
