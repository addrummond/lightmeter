#include <avr/io.h>

int main()
{
    DDRB = 0b11000;

    // We want to flip output at about 2500Hz. At 16MHz this means about once
    // every 6400 clock cycles.

    PORTB = 0;
    PORTB |= 0b1000;

    // Guessing at about 4 cycles overhead per counter increase.
    for (;;) {
        uint8_t i;
        for (i = 0; i < 100/4; ++i) {
             uint8_t j;
             for (j = 0; j < 64; ++j);
        }

        PORTB ^= 0b11000;
    }
}
