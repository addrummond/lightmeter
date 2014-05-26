#include <deviceconfig.h>

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <state.h>
#include <exposure.h>
#include <tables.h>
#include <display.h>
#include <ui.h>
#include <mymemset.h>

#include <basic_serial/basic_serial.h>

// Save result of ADC on interrupt.
volatile static uint16_t adc_light_value = 0;
volatile static uint16_t adc_temperature_value = 300; // About 25C
volatile static bool next_is_temperature = true;
ISR(ADC_vect) {
    if (next_is_temperature) {
        adc_temperature_value = ADCW;
    }
    else {
        adc_light_value = ADCW;
        global_transient_meter_state.exposure_ready = true;
    }

    TIFR |= (1<<OCF0A); // Clear timer compare match flag.
    ADCSRA |= (1 << ADSC);
}

// TODO: These constants will be replaced with members of global_meter_state once
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

static volatile uint8_t current_temp = 193; // 22 C

static void calculate_current_temp()
{
    uint16_t t = adc_temperature_value;
    uint8_t i;
    uint16_t newt = t - ADC_TEMP_CONSTANT;
    for (i = 0; i < ADC_TEMP_SLOPE_WHOLES; ++i)
        newt += t;
    uint16_t tenth = newt/10;
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
    ADCSRB |= ((0 << ADTS2) | (1 << ADTS1) | (1 << ADTS0)); // Timer/Counter0 Compare Match A
    TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));
    TCCR0B |= ((1 << CS02) | (0 << CS01) | (0 << CS00)); // prescaler: clock/256
    TCNT0 = 0;
    OCR0A = 4; // TO-----------

    // Turn off bipolar input mode.
    // N/A on attiny1634.
    //ADCSRB &= ~(1 << BIN);

    // Sets prescaler to 128, which at 8MHz gives a 62KHz ADC.
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // Disable digital input on ADC pin.
    ADC_LIGHT_DIDR |= (1 << ADC_LIGHT_DIDR_BIT);

    // Enable ADC.
    ADCSRA |= (1 << ADEN);
    // Start taking readings.
    ADCSRA |= (1 << ADSC);
}

static void set_op_amp_resistor_stage(uint8_t op_amp_resistor_stage)
{
    // TODO: Code that actually switches the MOFSETs.

    global_transient_meter_state.op_amp_resistor_stage = op_amp_resistor_stage;
}

void led_test(void);
void handle_measurement()
{
    // Copy the volatile value, which could change in the middle of this function.
    // Div by 4 because we're going from units of 1/1024 to units of 1/256.
    // (Could left adjust everything, but we might want the full 10 bits for temps.)
    uint8_t adc_light_nonvol_value = (uint8_t)(adc_light_value >> 2);

    global_transient_meter_state.last_ev_with_tenths = get_ev100_at_temperature_voltage(
        current_temp,
        adc_light_nonvol_value,
        global_transient_meter_state.op_amp_resistor_stage
    );

    if (global_meter_state.priority == SHUTTER_PRIORITY) {
        global_transient_meter_state.shutter_speed.ev = global_meter_state.priority_shutter_speed;
        global_transient_meter_state.shutter_speed.tenths = tenth_below_eighth(global_meter_state.priority_shutter_speed);
        global_transient_meter_state.aperture = aperture_given_shutter_speed_iso_ev(
            global_transient_meter_state.shutter_speed.ev,
            global_meter_state.stops_iso,
            global_transient_meter_state.last_ev_with_tenths
        );
    }
    else { //if (global_meter_state.priority == APERTURE_PRIORITY) {
        global_transient_meter_state.aperture.ev = global_meter_state.priority_aperture;
        global_transient_meter_state.aperture.tenths = tenth_below_eighth(global_meter_state.priority_aperture);
        global_transient_meter_state.shutter_speed =
            shutter_speed_given_aperture_iso_ev(
                global_transient_meter_state.aperture.ev,
                global_meter_state.stops_iso,
                global_transient_meter_state.last_ev_with_tenths
        );
    }

    // If we're too near the top or bottom of the range, change the gain next time.
    //    if (adc_light_nonvol_value > 250 && global_meter_state.op_amp_resistor_stage < NUM_OP_AMP_RESISTOR_STAGES) {
    //        set_op_amp_resistor_stage(global_meter_state.op_amp_resistor_stage + 1);
    //    }
    //    else if (adc_light_nonvol_value <= VOLTAGE_TO_EV_ABS_OFFSET + 5 && global_meter_state.op_amp_resistor_stage > 1) {
    //        set_op_amp_resistor_stage(global_meter_state.op_amp_resistor_stage - 1);
    //    }
}

void led_test()
{
#ifdef TEST_LED_PORT
    TEST_LED_PORT |= (1 << TEST_LED_BIT);
    _delay_ms(250);
    TEST_LED_PORT &= ~(1 << TEST_LED_BIT);
#endif
}

// Some compile-time sanity checks on the values of the resistors
// for the buttons.
struct dummy_deviceconfig_pushbutton_test_struct {
    // Check that resistor values go up in sequence.
    int dummy5[(PUSHBUTTON1_RVAL_KO < PUSHBUTTON2_RVAL_KO &&
                PUSHBUTTON2_RVAL_KO < PUSHBUTTON3_RVAL_KO &&
                PUSHBUTTON3_RVAL_KO < PUSHBUTTON4_RVAL_KO) - 1];
};

static volatile uint8_t last_button_voltage_reading;
ISR(PCINT0_vect)
{
    // We haven't finished handling the last button press yet, so return.
    if (last_button_voltage_reading)
        return;

    // Ignore if it's high rather than low.
    if (PUSHBUTTON_PIN & (1 << PUSHBUTTON_BIT))
        return;

    // We now need to switch the ADC temporarily to read from the button
    // input pin.
    ADMUX = PUSHBUTTON_ADMUX;
    // Disable ADC interrupt.
    ADCSRA &= ~(1 << ADIE);
    // Disable auto triggering.
    ADCSRA &= ~(1 << ADATE);
    // Start conversion.
    ADCSRA |= (1 << ADSC);

    // First conversion will be junk; ignore it.
    while (ADCSRA & (1 << ADSC));
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    uint8_t r = (uint8_t)(ADC >> 2);

    // Ensure that it's not zero (since zero signals that no button has been pressed).
    // The small loss of accuracy doesn't matter.
    last_button_voltage_reading = r | 1;

    // ADC is reset to the usual settings in the main loop, so as to
    // minimize the amount of processing done in this interrupt.
}

static void setup_button_handler()
{
    PUSHBUTTON_DDR &= ~(1 << PUSHBUTTON_BIT);
    GIMSK |= (1 << PCIE0);
    PUSHBUTTON_PCMSK |= (1 << PUSHBUTTON_PCINT);
    // Enable pullup resistor.
    PUSHBUTTON_PUE |= (1 << PUSHBUTTON_BIT);
}

static void handle_button_press(uint8_t lbvr)
{
#ifdef DEBUG
    tx_byte('B');
    tx_byte(lbvr);
#endif
    //    if (button_number == 1) {
    //    display_clear();
//    _delay_ms(2000);
        //    }
}

static void setup_charge_pump()
{
    // Might be worth investigating the possibility of getting the chip to flip
    // the ports automatically instead of using an interrupt. However, I couldn't
    // figure out any way of getting it to output to two ports simultaneously via
    // a counter. The following SO question may be helpful:
    //
    // http://electronics.stackexchange.com/questions/49852/need-help-understanding-avr-atmega-attiny-timer-mirrored-output

    CHARGE_PUMP_CLOCKS_DDR |= (1 << CHARGE_PUMP_CLOCK1_BIT) | (1 << CHARGE_PUMP_CLOCK2_BIT);
    // Set inital charge pump port outputs as opposites.
    CHARGE_PUMP_CLOCKS_PORT |= (1 << CHARGE_PUMP_CLOCK1_BIT);
    CHARGE_PUMP_CLOCKS_PORT &= ~(1 << CHARGE_PUMP_CLOCK2_BIT);

    TCCR1A = 0;
    TCCR1B = (0 << WGM13) | (1 << WGM12); // CTC OCR1A.
    // Prescale clock by 1/64.
    TCCR1B |= (0 << CS12) | (1 << CS11) | (1 << CS10);
    OCR1A = 4;
    // Enable timer counter interrupt.
    TIMSK |= (1 << OCIE1A);
}

ISR(TIM1_COMPA_vect)
{
    CHARGE_PUMP_CLOCKS_PORT ^= ((1 << CHARGE_PUMP_CLOCK1_BIT) | (1 << CHARGE_PUMP_CLOCK2_BIT));
}

int main()
{
    #ifdef TEST_LED_PORT
        TEST_LED_DDR      |= (1 << TEST_LED_BIT);
        led_test();
    #endif

#ifdef DEBUG
    tx_byte('I');
#endif

    setup_button_handler();
    setup_ADC();

    set_op_amp_resistor_stage(2);
    initialize_global_meter_state();

    setup_charge_pump();

    sei();

    display_init();
    display_clear();

    // The main loop. This looks at the latest exposure
    // reading every so often.
    uint8_t cnt;
    for (cnt = 0;; ++cnt) {
        // Check if a button was pressed.
        uint8_t lbvr = last_button_voltage_reading;
        if (lbvr) {
            handle_button_press(lbvr);
            last_button_voltage_reading = 0;
            // Ordinary ADC settings will have been overwritten to read voltage at
            // button input pin; reset them now.
            setup_ADC();
        }

        // Do lightmetery stuff.
        if (cnt == 0) {
            // Make the next ADC reading a temperature reading.
            ADMUX &= ADMUX_CLEAR_REF_VOLTAGE;
            ADMUX |= ADMUX_TEMP_REF_VOLTAGE;
            ADMUX |= ADMUX_TEMPERATURE_SOURCE; // 1111, so no need to clear bits first.
            next_is_temperature = true;
        }
        else if (cnt == 0b100000) {
            // TODO: Calling this seems to give rise to some kind of memory corruption bug.
            //calculate_current_temp();

            // Make the next ADC reading a light reading.
            ADMUX &= ADMUX_CLEAR_REF_VOLTAGE;
            ADMUX |= ADMUX_LIGHT_SOURCE_REF_VOLTAGE;
            ADMUX &= ADMUX_CLEAR_SOURCE;
            ADMUX |= ADMUX_LIGHT_SOURCE;
            next_is_temperature = false;
        }

        handle_measurement();
        ui_show_interface();

        _delay_ms(50);
    }

    return 0;
}
