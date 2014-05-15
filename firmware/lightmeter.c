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
    //_delay_ms(250);
    //TEST_LED_PORT &= ~(1 << TEST_LED_BIT);
#endif
}

static void show_interface()
{
    // Used to make measurements of display power consumption.
    /*uint8_t col, page;
    for (page = 0; page < 8; ++page) {
        display_command(DISPLAY_SET_COL_START_LOW + (col & 0xF));
        display_command(DISPLAY_SET_COL_START_HIGH + (col >> 4));
        display_command(DISPLAY_SET_PAGE_START + page);
        for (col = 0; col < 128; ++col) {
            DISPLAY_WRITE_DATA {
                //display_write_byte(0xFF);
                //display_write_byte(0xF0);
                //display_write_byte(0b11000000);
                display_write_byte(0b1000000);
            }
        }
    }
    return;*/

    uint8_t i;
    uint8_t out[6];
    memset8_zero(out, sizeof(out));
    ui_top_status_line_state_t state0;
    memset8_zero(&state0, sizeof(state0));
    for (i = 0; i < DISPLAY_LCDWIDTH; i += 6) {
        memset8_zero(out, sizeof(out));
        ui_top_status_line_at_6col(&state0, out, 1, i);
        display_write_page_array(out, 6, 1, i, 0);
    }

    uint8_t out2[24];
    memset8_zero(out2, sizeof(out2));
    ui_main_reading_display_state_t state;
    memset8_zero(&state, sizeof(state));
    //static uint8_t val = 0;
    for (i = 0; i < DISPLAY_LCDWIDTH; i += 8) {
        memset8_zero(out2, sizeof(out2));
        ui_main_reading_display_at_8col(&state, out2, 3, i);
        //out2[0] = val++;
        display_write_page_array(out2, 8, 3, i, 3);
    }

    ui_bttm_status_line_state_t state2;
    memset8_zero(out, sizeof(out));
    memset8_zero(&state2, sizeof(state2));
    for (i = 0; DISPLAY_LCDWIDTH - i >= 6; i += 6) {
        memset8_zero(out, sizeof(out));
        ui_bttm_status_line_at_6col(&state2, out, 1, i);
        display_write_page_array(out, 6, 1, i, 7);
    }
}

// Some compile-time sanity checks on the values of the resistors
// for the buttons.
struct dummy_deviceconfig_pushbutton_test_struct {
    int dummy1[PUSHBUTTON_RC_MS(1) - 1];
    int dummy2[PUSHBUTTON_RC_MS(2) - 1];
    int dummy3[PUSHBUTTON_RC_MS(3) - 1];
    int dummy4[PUSHBUTTON_RC_MS(4) - 1];

    // Check that resistor values go up in sequence.
    int dummy5[(PUSHBUTTON1_RVAL_KO < PUSHBUTTON2_RVAL_KO &&
                PUSHBUTTON2_RVAL_KO < PUSHBUTTON3_RVAL_KO &&
                PUSHBUTTON3_RVAL_KO < PUSHBUTTON4_RVAL_KO) - 1];
};

static volatile uint8_t mode_counter = 0;
static volatile uint8_t last_button_press = 0;
ISR(TIM0_COMPA_vect)
{
    // See http://forums.parallax.com/showthread.php/110209-Multiple-buttons-to-one-IO-pin, post by
    // virtuPIC for description of method used here.

    if (mode_counter == 0) {
        // Switch pin to input mode (with no pullup resistors).
        PUSHBUTTON_DDR &= ~(1 << PUSHBUTTON_BIT);
        PUSHBUTTON_PORT &= ~(1 << PUSHBUTTON_BIT);
        //PUSHBUTTON_PORT |= (1 << PUSHBUTTON_BIT);
        // Check if the pin has gone high.
        if (PUSHBUTTON_PIN & (1 << PUSHBUTTON_BIT)) {
#ifdef DEBUG
tx_byte('B');
tx_byte('!');
#endif
            // Switch to output mode to discharge cap.
            PUSHBUTTON_DDR |= (1 << PUSHBUTTON_BIT);
            PUSHBUTTON_PORT &= ~(1 << PUSHBUTTON_BIT);
        }
        else {
            return;
        }
    }
    else if (mode_counter == PUSHBUTTON_RC_MS(4)) {
        // All capacitors have now had enough time to discharge.
        // Switch back to input mode and see how long capacitor
        // takes to charge.
        PUSHBUTTON_DDR |= (1 << PUSHBUTTON_BIT);
        PUSHBUTTON_PORT &= ~(1 << PUSHBUTTON_BIT);
        //PUSHBUTTON_PORT |= (1 << PUSHBUTTON_BIT);
    }
    else if (mode_counter == PUSHBUTTON_RC_MS(4)*2 + 1) {
        // Looks like the cap wasn't charged; no button was pressed.
        // Back to square one.
        mode_counter = 0;
        return;
    }
    else if (mode_counter > PUSHBUTTON_RC_MS(4)) {
#ifdef DEBUG
tx_byte('B');
tx_byte('C');
#endif
        // Has it charged yet?
        if (PUSHBUTTON_PIN & (1 << PUSHBUTTON_BIT)) {
#ifdef DEBUG
tx_byte('B');
tx_byte('X');
#endif
            uint8_t but = 0;
            if (mode_counter <= PUSHBUTTON_RC_MS(4) + PUSHBUTTON_RC_MS(1))
                but = 1;
            else if (mode_counter <= PUSHBUTTON_RC_MS(4) + PUSHBUTTON_RC_MS(2))
                but = 2;
            else if (mode_counter <= PUSHBUTTON_RC_MS(4) + PUSHBUTTON_RC_MS(3))
                but = 3;
            else if (mode_counter <= PUSHBUTTON_RC_MS(4) + PUSHBUTTON_RC_MS(4))
                but = 4;

            if (but) {
                mode_counter = 0;
                last_button_press = but;
                return;
            }
        }
    }

    ++mode_counter;
}

static void setup_button_handler()
{
    // We want to call the interrupt every millisecond.
    // Prescale the clock by /1024.
    TCCR0B |= ((1 << CS12) | (0 << CS11) | (1 << CS10));
    // Count to 8 to get roughly every millisecond.
    TCCR0B |= ((0 << WGM12) | (1 << WGM12) | (0 << WGM11) | (0 << WGM10));
    OCR0A = 8;
    // Enable CTC interrupt.
    TIMSK |= (1 << OCIE0A);
}

static void handle_button_press(uint8_t button_number)
{
    //    if (button_number == 1) {
        display_clear();
        _delay_ms(2000);
        //    }
}

int main()
{
    setup_button_handler();

    set_op_amp_resistor_stage(2);

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

        // Check if a button was pressed.
        uint8_t button_number = last_button_press;
        if (button_number)
            handle_button_press(button_number);

        _delay_ms(50);
    }

    return 0;
}
