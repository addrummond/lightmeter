#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <usitwislave.h>

// PB 2: RST
// PB 1: Clck
// PB 4: Data

int main()
{
    DDRB = 0b100;
    PORTB |= 0b100;
    _delay_ms(1);
    PORTB &= ~0b100;
    _delay_ms(10);
    PORTB |= 0b100;

    for (;;)
        _delay_ms(50);
}
