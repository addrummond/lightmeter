#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/*
                         1 ---- 8         VCC
     ADC neg in    (PB3) 2 ---- 7 (PB2)   LED right   (least sig)
     ADC pos in    (PB4) 3 ---- 6 (PB1)   LED 2
            GND          4 ---- 5 (PB0)   LED left    (most sig)
 */

// Save result of ADC on interrupt.
volatile static uint16_t adc_value = 0;
ISR(ADC_vect) {
    adc_value = ADCW;
}

void setupADC()
{
    // Set ref voltage to VCC.
    ADMUX = (0 << REFS1) | (0 << REFS0);
    // Set differential ADC with PB4 pos, PB3 neg, and 1x gain (see atiny85 datasheet p. 135).
    ADMUX |= (0 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0); // 0110
    // Auto triggering (this needs to be set for turning on free running in ADCSRB to take effect).
    ADCSRA |= (1 << ADATE);
    // Enable ADC interrupt.
    ADCSRA |= (1 << ADIE);
    // Free running mode.
    ADCSRB &= ~((1 << ADTS2) | (1 << ADTS1) | (1 << ADTS0));
    // Turn off bipolar input mode.
    ADCSRB &= ~(1 << BIN);
    // Sets prescaler to 128, which at 8MHz gives a 62KHz ADC.
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    // Left adjust ADC result to allow easy 8 bit reading 
    ADMUX |= (1 << ADLAR); 

    // Enable ADC.
    ADCSRA |= (1 << ADEN);
    // Start taking readings.
    ADCSRA |= (1 << ADSC);

    // Enable global interrupts.
    sei();
}

void setupOutputPorts()
{
    DDRB = 0b00000111; // Set PB2 - PB0 as output ports.
}

void handleMeasurement()
{
    uint8_t adch = adc_value >> 8;
    uint8_t outOfEight = adch >> 5;
    uint8_t led = ((outOfEight & 0b100)>> 2) | (outOfEight & 0b010) | ((outOfEight & 0b001) << 2);
    PORTB &= ~(0b111);
    PORTB |= led;
}


void ledTest()
{
    _delay_ms(500);
    PORTB |= 0b1;
    _delay_ms(500);
    PORTB &= ~(0b1);
    PORTB |= 0b10;
    _delay_ms(500);
    PORTB &= ~(0b10);
    PORTB |= 0b100;
    _delay_ms(500);
    PORTB &= ~(0b100);
    _delay_ms(500);
}

int main()
{
    setupOutputPorts();
    ledTest();

    _delay_ms(1000);

    setupADC();
    for (;;) {
        handleMeasurement();
        _delay_ms(250);
    }

    return 0;
}
