#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <usbdrv.h>

#include <state.h>
#include <usbconstants.h>
#include <calculate.h>
#include <exposure.h>

/*
                       1 ---- 8         VCC
     ADC in      (PB3) 2 ---- 7 (PB2)   USB D+
     Gain switch (PB4) 3 ---- 6 (PB1)   LED
     GND               4 ---- 5 (PB0)   USB D-

     Setting PB4 high allows one of the resistors to be skipped, reducing
     op amp gain.
 */

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
        178,  // 178 = 20C
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

const uchar testbuffer[] = "Hello world test message consisting of 64 bytes................."; // length 64

static uint8_t last_brequest;
static uint8_t usb_input_buffer[64];
static uint8_t usb_input_buffer_current_position;
static uint8_t usb_input_buffer_bytes_remaining;

USB_PUBLIC uchar usbFunctionSetup(uchar setupData[8]) {
    usbRequest_t *rq = (void *)setupData;

    switch (rq->bRequest) {
    case USB_BREQUEST_GET_TEST_MSG: {
        usbMsgLen_t len = 64;
        if (len > rq->wLength.word)
            len = rq->wLength.word;
        usbMsgPtr = (usbMsgPtr_t)testbuffer;
        return len;
    } break;
    case USB_BREQUEST_GET_EV: {
        usbMsgPtr = (usbMsgPtr_t)(&last_ev_reading); // This is volatile, but I think it's ok.
        return 1;
    } break;
    case USB_BREQUEST_SET_GAIN: {
        set_gain(rq->wValue.bytes[0] ? HIGH_GAIN : NORMAL_GAIN);
    } break;
    case USB_BREQUEST_GET_GAIN: {
        usbMsgPtr = (usbMsgPtr_t)(&(global_meter_state.gain));
        return 1;
    } break;
    case USB_BREQUEST_GET_SHUTTER_PRIORITY_EXPOSURE: {
        if (! global_meter_state.aperture_string.length) // No exposure calculated yet.
            return 0;
        usbMsgPtr = (usbMsgPtr_t)(APERTURE_STRING_OUTPUT_STRING(global_meter_state.aperture_string));
        return global_meter_state.aperture_string.length + 1; // Include '\0' terminator.
    } break;
    case USB_BREQUEST_SET_ISO: {
        last_brequest = rq->bRequest;
        usb_input_buffer_current_position = 0;
        usb_input_buffer_bytes_remaining = rq->wLength.word;
        if (usb_input_buffer_bytes_remaining > sizeof(usb_input_buffer))
            usb_input_buffer_bytes_remaining = sizeof(usb_input_buffer);
        return USB_NO_MSG; // Go to usbFunctionWrite
    } break;
    case USB_BREQUEST_GET_STOPS_ISO: {
        usbMsgPtr = (usbMsgPtr_t)(&(global_meter_state.stops_iso));
        return 1;
    } break;
    case USB_BREQUEST_SET_SHUTTER_SPEED: {
        global_meter_state.shutter_speed = rq->wValue.bytes[0];
        global_meter_state.priority = SHUTTER_PRIORITY;
        return 0;
    } break;
    case USB_BREQUEST_SET_APERTURE: {
        global_meter_state.aperture = rq->wValue.bytes[0];
        global_meter_state.priority = APERTURE_PRIORITY;
        return 0;
    } break;
    case USB_BREQUEST_GET_RAW_TEMPERATURE: {
        usbMsgPtr = (usbMsgPtr_t)(&adc_temperature_value);
        return 2;
    } break;
    case USB_BREQUEST_GET_RAW_LIGHT: {
        usbMsgPtr = (usbMsgPtr_t)(&adc_light_value);
        return 2;
    } break;
    }

    return 0;
}

USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len)
{
    if (len > usb_input_buffer_bytes_remaining)
        len = usb_input_buffer_bytes_remaining;

    usb_input_buffer_bytes_remaining -= len;
    uint8_t i;
    for (i = 0; i < len; ++i)
        usb_input_buffer[usb_input_buffer_current_position++] = data[i];

    if (usb_input_buffer_bytes_remaining != 0)
        return 0;

    switch (last_brequest) {
    case USB_BREQUEST_SET_ISO: {
        global_meter_state.bcd_iso_length = usb_input_buffer_current_position;
        for (i = 0; i < usb_input_buffer_current_position; ++i) {
            global_meter_state.bcd_iso[i] = usb_input_buffer[i];
        }
        string_to_bcd(global_meter_state.bcd_iso, usb_input_buffer_current_position);

        global_meter_state.stops_iso = iso_bcd_to_stops(global_meter_state.bcd_iso, global_meter_state.bcd_iso_length);
    } break;
    }

    return 1;
}

int main()
{
    initialize_global_meter_state();
    setup_output_ports();
    set_gain(global_meter_state.gain);
    setup_ADC();

    led_test();

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

    // The main loop. This handles USB polling and looks at the latest exposure
    // reading every so often.
    uint8_t cnt;
    for (cnt = 0;; ++cnt) {
        wdt_reset();
        usbPoll();

        if ((cnt & 0b11111) == 0) { // Every 160ms, give or take.
            // Do lightmetery stuff.
            if (cnt == 0) {
                // Make the next ADC reading a temperature reading.
                ADMUX &= ADMUX_CLEAR_REF_VOLTAGE;
                ADMUX |= ADMUX_TEMP_REF_VOLTAGE;
                ADMUX |= ADMUX_TEMPERATURE_SOURCE; // 1111, so no need to clear bits first.
                next_is_temperature = true;
            }
            else if (cnt == 0b100000) {
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
            // usbPoll needs to be called at least every 50ms. Tried using higher
            // values < 50 than the one here but they seemed to make reading data
            // from the device noticeably slow.
            _delay_ms(10);
        }
    }

    return 0;
}
