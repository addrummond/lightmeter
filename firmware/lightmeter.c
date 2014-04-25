#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <state.h>
#include <calculate.h>
#include <exposure.h>
#include <divmulutils.h>

const uint8_t ADMUX_CLEAR_SOURCE = ~((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0));

// Set differential ADC with PB4 pos, PB3 neg, and 1x gain (see atiny85 datasheet p. 135).
//const uint8_t ADMUX_LIGHT_SOURCE = (0 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0); // 0110

// Set single-ended ADC with PB3 as input.
const uint8_t ADMUX_LIGHT_SOURCE = (0 << MUX3) | (0 << MUX2) | (1 << MUX1) | (1 << MUX0);

const uint8_t ADMUX_TEMPERATURE_SOURCE = (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0);

const uint8_t ADMUX_CLEAR_REF_VOLTAGE = 0b00101111; // Couldn't do this with macros without getting overflow warning for some reason.
const uint8_t ADMUX_TEMP_REF_VOLTAGE = (0 << REFS2) | (1 << REFS1) | (0 << REFS0); // 1.1V internal reference
const uint8_t ADMUX_LIGHT_SOURCE_REF_VOLTAGE = (0 << REFS2) | (0 << REFS1) | (0 << REFS0); // VCC

volatile static uint8_t last_ev_reading = 0;

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

// TODO: These constant will be replaced with members of global_meter_state once
// this is working correctly. The various _TENTHS could be packed into two bytes.
//
// For current test chip, slope = 1.12 and k = 290
// We're going from x to y, 1/slope = 0.89.
// After 
#define ADC_TEMP_CONSTANT              785
#define ADC_TEMP_SLOPE_WHOLES          0 // I.e. add one more whole.
#define ADC_TEMP_SLOPE_EIGHT_TENTHS    0
#define ADC_TEMP_SLOPE_FOUR_TENTHS     1
#define ADC_TEMP_SLOPE_TENTHS          0
#define ADC_TEMP_SLOPE_MINUS_TENTHS    0

static volatile uint8_t current_temp = 190; // 25 C

static void calculate_current_temp()
{
    uint16_t t = adc_temperature_value;
    uint8_t i;
    uint16_t newt = t - ADC_TEMP_CONSTANT;
    for (i = 0; i < ADC_TEMP_SLOPE_WHOLES; ++i)
        newt += t;
    uint16_t tenth = bitfiddle_uint16_approx_div_by_10(newt);
    for (i = 0; i < ADC_TEMP_SLOPE_TENTHS; ++i)
        newt += tenth;
    for (i = 0; i < ADC_TEMP_SLOPE_MINUS_TENTHS; ++i)
        newt -= tenth;
    tenth <<= 2;
    for (i = 0; i < ADC_TEMP_SLOPE_FOUR_TENTHS; ++i)
        newt += tenth;
    tenth <<= 1;
    for (i = 0; i < ADC_TEMP_SLOPE_EIGHT_TENTHS; ++i)
        newt += tenth;

    current_temp = newt;
}

void setup_ADC()
{

    ADMUX = (0 << REFS1) | (0 << REFS0);
    // Set ADC source to light sensor.
    ADMUX |= ADMUX_LIGHT_SOURCE | ADMUX_LIGHT_SOURCE_REF_VOLTAGE;
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
    DDRB = 0b00010010; // Set PB1 and PB4 as output ports.
}

void led_test(void);
void handle_measurement()
{
    // Copy the volatile value, which could change in the middle of this function.
    uint16_t adc_light_nonvol_value = adc_light_value;

    // Div by 4 because we're going from units of 1/1024 to units of 1/256.
    // (Could left adjust everything, but we might want the full 10 bits for temps.)
    uint8_t ev = get_ev100_at_temperature_voltage(
        current_temp,
        (uint8_t)(adc_light_nonvol_value >> 2),
        global_meter_state.gain
    );

    last_ev_reading = ev;

    if (global_meter_state.priority == SHUTTER_PRIORITY) {
        uint8_t ap = aperture_given_shutter_speed_iso_ev(global_meter_state.shutter_speed, global_meter_state.stops_iso, ev);
        aperture_to_string(ap, &(global_meter_state.aperture_string));
    }
    else if (global_meter_state.priority == APERTURE_PRIORITY) {
        uint8_t shut = shutter_speed_given_aperture_iso_ev(global_meter_state.aperture, global_meter_state.stops_iso, ev);
        shutter_speed_to_string(shut, &(global_meter_state.shutter_speed_string));
    }
}

void led_test()
{
    PORTB |= (0b10);
    _delay_ms(500);
    PORTB &= ~(0b10);
}

void set_gain(gain_t gain)
{
    PORTB &= ~(0b1000);
    if (gain == HIGH_GAIN)
        PORTB |= 0b1000;

    global_meter_state.gain = gain;
}

int main()
{
    initialize_global_meter_state();
    setup_output_ports();
    set_gain(global_meter_state.gain);
    setup_ADC();

    led_test();

    uint8_t i;
    for (i = 0; i < 250; ++i) {
        wdt_reset();
        _delay_ms(2);
    }

    sei();

    // The main loop. This looks at the latest exposure
    // reading every so often.
    uint8_t cnt;
    for (cnt = 0;; ++cnt) {
        if ((cnt & 0b111) == 0) { // Every 700ms, give or take.
            // Do lightmetery stuff.
            if (cnt == 0) {
                // Make the next ADC reading a temperature reading.
                ADMUX &= ADMUX_CLEAR_REF_VOLTAGE;
                ADMUX |= ADMUX_TEMP_REF_VOLTAGE;
                ADMUX |= ADMUX_TEMPERATURE_SOURCE; // 1111, so no need to clear bits first.
                next_is_temperature = true;
            }
            else if (cnt == 0b100000) {
                calculate_current_temp();

                // Make the next ADC reading a light reading.
                ADMUX &= ADMUX_CLEAR_REF_VOLTAGE;
                ADMUX |= ADMUX_LIGHT_SOURCE_REF_VOLTAGE;
                ADMUX &= ADMUX_CLEAR_SOURCE;
                ADMUX |= ADMUX_LIGHT_SOURCE;
                next_is_temperature = false;
            }

            handle_measurement();

            // A bit less often still, save settings to EEPROM.
            // TODO: Not sure about the power consumption implications of writing to EEPROM
            // this frequently. Will it use a lot of power? Or wear out the EEPROM quickly?
            if (cnt == 255) {
                write_meter_state(&global_meter_state);
            }
        }
        else {
            _delay_ms(100);
        }
    }

    return 0;
}
