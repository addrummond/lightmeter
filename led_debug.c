#include <led_debug.h>
#include <avr/io.h>
#include <util/delay.h>

// Display a byte value using three LEDs.
void debug_led_show_byte(uint8_t byte)
{
    uint8_t leds[3] = { byte >> 5, (byte & 0b00011100) >> 2, byte & 0b00000011 };

    // Two brief flashes of all the LEDs to signal the start of the value.
    PORTB |= ((1 << LED_LEFT_PB | (1 << LED_MIDDLE_PB) | (1 << LED_RIGHT_PB)));
    _delay_ms(100);
    PORTB &= ~((1 << LED_LEFT_PB | (1 << LED_MIDDLE_PB) | (1 << LED_RIGHT_PB)));
    _delay_ms(100);
    PORTB |= ((1 << LED_LEFT_PB | (1 << LED_MIDDLE_PB) | (1 << LED_RIGHT_PB)));
    _delay_ms(100);

    int i;
    for (i = 0; i < 3; ++i) {
        PORTB &= ~((1 << LED_LEFT_PB | (1 << LED_MIDDLE_PB) | (1 << LED_RIGHT_PB)));
        PORTB |= (((leds[i] & 4) << LED_RIGHT_PB) |
                  ((leds[i] & 2) << LED_MIDDLE_PB) |
                  ((leds[i] & 1) << LED_LEFT_PB));
        _delay_ms(1000);
    }
}
