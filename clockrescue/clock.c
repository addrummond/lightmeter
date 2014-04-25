#include <avr/io.h>
#include <avr/cpufunc.h>

int main()
{
    for (;;) {
        DDRB = 0xFF;
        PORTB = ~PINB;
        _NOP();
        _NOP();
    }
}

