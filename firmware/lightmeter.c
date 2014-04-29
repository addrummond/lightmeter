#include <deviceconfig.h>

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
#include <tables.h>
#include <display.h>
#include <ui.h>
#include <mymemset.h>

const uint8_t ADMUX_CLEAR_SOURCE = ~((1 << MUX5) | (1 << MUX4) | (1 << MUX2) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0));
const uint8_t ADMUX_CLEAR_REF_VOLTAGE = 0b11000000; // Couldn't do this with macros without getting overflow warning for some reason.

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

    TIFR0 |= (1<<OCF0A); // Clear timer compare match flag.
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
    uint16_t tenth = (uint16_t)bitfiddle_uint16_to_uint8_div_by_10(newt);
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
    // Output DDR for display handled in display_init.
#ifdef TEST_LED_PORT
    TEST_LED_DDR      |= (1 << TEST_LED_BIT);
#endif
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
    
    uint8_t ev = get_ev100_at_temperature_voltage(
        current_temp,
        adc_light_nonvol_value,
        global_transient_meter_state.op_amp_resistor_stage
    );
    global_transient_meter_state.last_ev = ev;

    uint8_t ap, shut;
    if (global_meter_state.priority == SHUTTER_PRIORITY) {
        shut = global_transient_meter_state.shutter_speed;
        ap = aperture_given_shutter_speed_iso_ev(shut, global_meter_state.stops_iso, ev);
    }
    else { //if (global_meter_state.priority == APERTURE_PRIORITY) {
        ap = global_transient_meter_state.aperture;
        shut = shutter_speed_given_aperture_iso_ev(ap, global_meter_state.stops_iso, ev);
    }
    shutter_speed_to_string(shut, &(global_transient_meter_state.shutter_speed_string));
    aperture_to_string(ap, &(global_transient_meter_state.aperture_string));

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

static void show_interface()
{
    uint8_t i;
    uint8_t out[6];
    ui_top_status_line_state_t state0;
    memset8_zero(&state0, sizeof(state0));
    for (i = 0; i < DISPLAY_LCDWIDTH; i += 6) {
        memset8_zero(out, sizeof(out));
        ui_top_status_line_at_6col(&state0, out, 1, i);
        display_write_page_array(out, 6, 1, i, 0);
    }

    uint8_t out2[24];
    ui_main_reading_display_state_t state;
    memset8_zero(&state, sizeof(state));
    for (i = 0; i < DISPLAY_LCDWIDTH; i += 8) {
        memset8_zero(out2, sizeof(out2));
        ui_main_reading_display_at_8col(&state, out2, 3, i);
        display_write_page_array(out2, 8, 3, i, 3);
    }

    ui_bttm_status_line_state_t state2;
    memset8_zero(&state2, sizeof(state2));
    for (i = 0; DISPLAY_LCDWIDTH - i >= 6; i += 6) {
        memset8_zero(out, sizeof(out));
        ui_bttm_status_line_at_6col(&state2, out, 1, i);
        display_write_page_array(out, 6, 1, i, 7);
    }
}

static void setup_charge_pump()
{
    // Set initial charge pump clocks in inverse phase.
    DDRA |= ((1 << CHARGE_PUMP_CLOCK1_BIT) | (1 << CHARGE_PUMP_CLOCK2_BIT));
    CHARGE_PUMP_CLOCKS_PORT |= (1 << CHARGE_PUMP_CLOCK1_BIT);
    CHARGE_PUMP_CLOCKS_PORT &= ~(1 << CHARGE_PUMP_CLOCK2_BIT);

    // Prescale clock by /1024. This gives us a 7.812KHz counter.
    TCCR1B |= ((1 << CS12) | (0 << CS11) | (1 << CS10));
    // We want 4KHz, so we count to two.
    TCCR1B |= ((0 << WGM13) | (1 << WGM12) | (0 << WGM11) | (0 << WGM10));
    OCR1A = 2; // Count to two.
    // Enable CTC interrupt.
    TIMSK1 |= (1 << OCIE1A);
}

static volatile uint8_t mode_counter = 0;
static volatile uint8_t last_button_press = 0;
ISR(TIM0_COMPA_vect)
{
    if (mode_counter == 0) {
        // Charge capacitor (if one of the buttons is pressed).
        PUSHBUTTON_PORT |= (1 << PUSHBUTTON_BIT);
    }
    else if (mode_counter >= PUSHBUTTON_RC_MS(4)) {
        // Capacitor has now had enough time to charge through any of the resistors.
        // Switch pin to input mode so we can time discharge (if any).
        PUSHBUTTON_DDR &= ~(1 << PUSHBUTTON_BIT);
    }
    else if (mode_counter <= PUSHBUTTON_RC_MS(4) + PUSHBUTTON_RC_MS(4)) {
        // See if the pin has gone low yet.
        if (PUSHBUTTON_PIN ^ (1 << PUSHBUTTON_BIT)) {
            if (mode_counter == PUSHBUTTON_RC_MS(4) + PUSHBUTTON_RC_MS(1)) {
                last_button_press = 1;
            }
            else if (mode_counter == PUSHBUTTON_RC_MS(4) + PUSHBUTTON_RC_MS(2)) {
                last_button_press = 2;
            }
            else if (mode_counter == PUSHBUTTON_RC_MS(4) + PUSHBUTTON_RC_MS(3)) {
                last_button_press = 3;
            }
            else if (mode_counter == PUSHBUTTON_RC_MS(4) + PUSHBUTTON_RC_MS(4)) {
                last_button_press = 4;
            }

            mode_counter = 0;
            return;
        }
    }

    ++mode_counter;
}

static void setup_button_handler()
{
    // Set up for output initially.
    PUSHBUTTON_DDR |= (1 << PUSHBUTTON_BIT);
    // We want to call the interrupt every two milliseconds.
    // Prescale the clock by /1024.
    TCCR0B |= ((1 << CS12) | (0 << CS11) | (1 << CS10));
    // Count to 15 to get roughly every two milliseconds.
    TCCR0B |= ((0 << WGM12) | (1 << WGM12) | (0 << WGM11) | (0 << WGM10));
    OCR0A = 15;
    // Enable CTC interrupt.
    TIMSK0 |= (1 << OCIE0A);
}

// Interrupt for driving charge pump clocks.
ISR(TIM1_COMPA_vect)
{
    CHARGE_PUMP_CLOCKS_PORT ^= ((1 << CHARGE_PUMP_CLOCK1_BIT) | (1 << CHARGE_PUMP_CLOCK2_BIT));
}

int main()
{
    setup_charge_pump();
    setup_button_handler();

    // TEST INITIALIZATION; TODO REMOVE EVENTUALLY.
    global_transient_meter_state.shutter_speed = 88;
    global_transient_meter_state.aperture = 88;
    //    global_transient_meter_state.exposure_ready = true;
    set_op_amp_resistor_stage(2);

    led_test();
    setup_output_ports();
    led_test();

    initialize_global_meter_state();
    display_init();
    display_clear();
    setup_ADC();

    sei();

    // The main loop. This looks at the latest exposure
    // reading every so often.
    uint8_t cnt;
    for (cnt = 0;; ++cnt) {
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
        show_interface();

        _delay_ms(50);
    }

    return 0;
}
