#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <usbdrv.h>

#include <constants.h>
#include <calculate.h>

/*
                         1 ---- 8         VCC
     ADC neg in    (PB3) 2 ---- 7 (PB2)   LED right   (least sig)
     ADC pos in    (PB4) 3 ---- 6 (PB1)   LED 2
            GND          4 ---- 5 (PB0)   LED left    (most sig)
 */

const uint8_t ADMUX_CLEAR_SOURCE = ~((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0));
// Set differential ADC with PB4 pos, PB3 neg, and 1x gain (see atiny85 datasheet p. 135).
const uint8_t ADMUX_LIGHT_SOURCE = (0 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0); // 0110
const uint8_t ADMUX_TEMPERATURE_SOURCE = (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0);

const uint8_t ADMUX_CLEAR_REF_VOLTAGE = 0b00101111; // Couldn't do this with macros without getting overflow warning for some reason.
const uint8_t ADMUX_TEMP_REF_VOLTAGE = (0 << REFS2) | (1 << REFS1) | (0 << REFS0); // 1.1V internal reference
const uint8_t ADMUX_LIGHT_SOURCE_REF_VOLTAGE = (0 << REFS2) | (0 << REFS1) | (0 << REFS0); // VCC

// Save result of ADC on interrupt.
volatile static uint16_t adc_light_value = 0;
volatile static uint16_t adc_temperature_value = 300; // About 25C
volatile static bool next_is_temperature = true;
ISR(ADC_vect) {
    if (next_is_temperature)
        adc_temperature_value = ADCW;
    else
        adc_light_value = ADCW;

    TIFR |= (1<<OCF0A); // Clear timer compare match flag.
    ADCSRA |= (1 << ADSC);
}

void setup_ADC()
{

    ADMUX = (0 << REFS1) | (0 << REFS0);
    // Set ADC source to temperature sensor.
    //ADMUX |= ADMUX_TEMPERATURE_SOURCE;
    //    ADMUX |= ADMUX_TEMP_REF_VOLTAGE;
    ADMUX |= ADMUX_LIGHT_SOURCE | ADMUX_LIGHT_SOURCE_REF_VOLTAGE; // TODO TEST CODE TODO
    // Auto triggering (this needs to be set for turning on counter interrupt ADCSRB to take effect).
    ADCSRA |= (1 << ADATE);
    // Enable ADC interrupt.
    ADCSRA |= (1 << ADIE);
    // Timer/counter 0 compare match A setup.
    // This causes the ADC to fire on an interupt once every 1024 clock cycles
    // (so about 8 times a second if we're running at 8MHz).
    ADCSRB &= ~((1 << ADTS2) | (1 << ADTS1) | (1 << ADTS0));
    ADCSRB &= ~((1 << ADTS2) | (1 << ADTS1) | (1 << ADTS0));
    ADCSRB |= ((0 << ADTS2) | (1 << ADTS1) | (1 << ADTS0)); // Timer/Counter0 Compare Match A
    TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));
    TCCR0B |= ((1 << CS02) | (0 << CS01) | (0 << CS00)); // prescaler: clock/256
    TCNT0 = 0;
    OCR0A = 4; // TO-----------
    // Turn off bipolar input mode.
    ADCSRB &= ~(1 << BIN);
    // Sets prescaler to 128, which at 8MHz gives a 62KHz ADC.
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // Enable ADC.
    ADCSRA |= (1 << ADEN);
    // Start taking readings.
    ADCSRA |= (1 << ADSC);

    // Enable global interrupts.
    sei();
}

void setup_output_ports()
{
    DDRB = 0b00000111; // Set PB2 - PB0 as output ports.
}

void led_test(void);
void handle_measurement()
{
    // Copy the volatile value, which could change in the middle of this function.
    uint16_t adc_light_nonvol_value = adc_light_value;

    uint8_t v = convert_from_reference_voltage(adc_light_nonvol_value);
    uint8_t ev = get_ev100_at_temperature_voltage(178, v); // 128 = 20C
    // Bit of a hack.
    uint8_t out_of_eight = ev >> 5;

    uint8_t led = ((out_of_eight & 0b100) >> 2) | (out_of_eight & 0b010) | ((out_of_eight & 0b001) << 2);
    //    PORTB &= ~(0b111);
    //    PORTB |= led;
}

void led_test()
{
    PORTB &= ~(0b111);
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

USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
    return 0;
}

int main()
{
    //    setup_output_ports();

    wdt_enable(WDTO_1S);

    usbInit();

    usbDeviceDisconnect();
    uint8_t i;
    for (i = 0; i < 250; ++i) {
        wdt_reset();
        _delay_ms(2);
    }
    usbDeviceConnect();

    sei();

    for (;;) {
        wdt_reset();
        usbPoll();
    }

    return 0;

    /*
    setup_output_ports();
    led_test();

    _delay_ms(1000);

    setup_ADC();

    uint8_t counter;
    for (counter = 0;; counter += 4) { // Will overflow every 64 loops, so about every 16 seconds.
        // This looks like it ought not to work reliably, since we might be changing
        // ADMUX in the middle of an ADC conversion. However, the docs say that the
        // ADMUX register is buffered so that updates only take effect when it is safe.
        // This code relies on the assumption that during any given call to the ADC
        // interrupt the values of ADCL and ADCH will always be the values for the
        // measurement specified in the most recent update to ADMUX.

        if (counter == 0) {
            // Make the next ADC reading a temperature reading.
            ADMUX &= ADMUX_CLEAR_REF_VOLTAGE;
            ADMUX |= ADMUX_TEMP_REF_VOLTAGE;
            ADMUX |= ADMUX_TEMPERATURE_SOURCE; // 1111, so no need to clear bits first.
            next_is_temperature = true;
        }
        else if (counter == 4) {
            // Make the next ADC reading a light reading.
            ADMUX &= ADMUX_CLEAR_REF_VOLTAGE;
            ADMUX |= ADMUX_LIGHT_SOURCE_REF_VOLTAGE;
            ADMUX &= ADMUX_CLEAR_SOURCE;
            ADMUX |= ADMUX_LIGHT_SOURCE;
            next_is_temperature = false;
        }

        handle_measurement();
        _delay_ms(250);
    }
    
    return 0;
    */
}
