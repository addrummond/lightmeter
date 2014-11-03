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
#include <shiftregister.h>

#ifdef DEBUG
#include <basic_serial/basic_serial.h>
#endif

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

static void calculate_current_temp(uint16_t t)
{
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

    //
    // ***** No longer using interrupts for ADC *****
    //
    //
    // Auto triggering (this needs to be set for turning on counter interrupt ADCSRB to take effect).
    //ADCSRA |= (1 << ADATE);
    // Enable ADC interrupt.
    //ADCSRA |= (1 << ADIE);
    // Timer/counter 0 compare match A setup.
    // This causes the ADC to fire on an interupt once every 1024 clock cycles
    // (so about 8 times a second if we're running at 8MHz).
    //ADCSRB &= ~((1 << ADTS2) | (1 << ADTS1) | (1 << ADTS0));
    //ADCSRB |= ((0 << ADTS2) | (1 << ADTS1) | (1 << ADTS0)); // Timer/Counter0 Compare Match A
    //TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));
    //TCCR0B |= ((1 << CS02) | (0 << CS01) | (0 << CS00)); // prescaler: clock/256
    //TCNT0 = 0;
    //OCR0A = 4; // TO-----------

    // Turn off bipolar input mode.
    // N/A on attiny1634.
    //ADCSRB &= ~(1 << BIN);

    // Sets prescaler to 128, which at 8MHz gives a 62KHz ADC.
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // Disable digital input on ADC pin.
    ADC_LIGHT_DIDR |= (1 << ADC_LIGHT_DIDR_BIT);

    // Enable ADC.
    ADCSRA |= (1 << ADEN);
}

#define DIODE_INCIDENT_NOND   SHIFT_REGISTER_DIODESW1_BIT
#define DIODE_INCIDENT_ND     SHIFT_REGISTER_DIODESW2_BIT
#define DIODE_REFLECTIVE_NOND SHIFT_REGISTER_DIODESW3_BIT
#define DIODE_REFLECTIVE_ND   SHIFT_REGISTER_DIODESW4_BIT
#define DIODE_SAME            255
static void set_diode_configuration(uint8_t op_amp_resistor_stage, uint8_t diode)
{
    op_amp_resistor_stage -= 1;

    // Select diode.
    if (diode != DIODE_SAME) {
        and_shift_register_bits(~((1 << SHIFT_REGISTER_DIODESW1_BIT) | (1 << SHIFT_REGISTER_DIODESW2_BIT) |
                                  (1 << SHIFT_REGISTER_DIODESW3_BIT) | (1 << SHIFT_REGISTER_DIODESW4_BIT)));
        or_shift_register_bits(1 << diode);
    }

    // Select op amp resistor stage.
    and_shift_register_bits(~((1 << SHIFT_REGISTER_STG1_BIT) || (1 << (SHIFT_REGISTER_STG1_BIT+1))));
    uint8_t stg1 = op_amp_resistor_stage & 1;
    or_shift_register_bits(stg1 << SHIFT_REGISTER_STG1_BIT);
    uint8_t stg2 = (op_amp_resistor_stage & 2)>>1;
    or_shift_register_bits(stg2 << (SHIFT_REGISTER_STG1_BIT+1));
    set_shift_register_out();
    global_transient_meter_state.op_amp_resistor_stage = op_amp_resistor_stage;
}

static uint16_t get_adc_reading()
{
    ADCSRA |= (1<<ADSC);
    while(ADCSRA & (1<<ADSC));
    return ADCW;
}

void handle_measurement(uint16_t adc_light_value)
{
    // Div by 4 because we're going from units of 1/1024 to units of 1/256.
    // (Could left adjust everything, but we might want the full 10 bits for temps.)
    uint8_t light_value8 = (adc_light_value >> 2);

    global_transient_meter_state.last_ev_with_fracs = get_ev100_at_temperature_voltage(
        current_temp,
        light_value8,
        global_transient_meter_state.op_amp_resistor_stage
    );

    if (global_meter_state.priority == SHUTTER_PRIORITY) {
        global_transient_meter_state.shutter_speed = global_meter_state.priority_shutter_speed;
        global_transient_meter_state.aperture = aperture_given_shutter_speed_iso_ev(
            global_transient_meter_state.shutter_speed,
            global_meter_state.stops_iso,
            global_transient_meter_state.last_ev_with_fracs
        );
    }
    else { //if (global_meter_state.priority == APERTURE_PRIORITY) {
        global_transient_meter_state.aperture = global_meter_state.priority_aperture;
        global_transient_meter_state.shutter_speed = shutter_speed_given_aperture_iso_ev(
            global_transient_meter_state.aperture,
            global_meter_state.stops_iso,
            global_transient_meter_state.last_ev_with_fracs
        );
    }

    static uint8_t debounce = 0;

    // If we're too near the top or bottom of the range, change the gain next time.
    uint8_t newr = 0; // Not a valid stage.
    if (light_value8 > 250 && global_transient_meter_state.op_amp_resistor_stage < NUM_OP_AMP_RESISTOR_STAGES) {
        newr = global_transient_meter_state.op_amp_resistor_stage + 1;
    }
    else if (light_value8 <= VOLTAGE_TO_EV_ABS_OFFSET + 5 && global_transient_meter_state.op_amp_resistor_stage > 1) {
        newr = global_transient_meter_state.op_amp_resistor_stage - 1;
    }

    if (newr) {
        if (debounce > 0) {
            --debounce;
        }
        else {
            set_diode_configuration(newr, DIODE_SAME);
            debounce = 8;
        }
    }
}

static void setup_pushbuttons()
{
#define SETUP(n) PUSHBUTTON ## n ## _DDR &= ~(1 << PUSHBUTTON ## n ## _BIT); \
                 GIMSK |= (1 << PUSHBUTTON ## n ## _PCIE); \
                 PUSHBUTTON ## n ## _PCMSK |= (1 << PUSHBUTTON ## n ## _PCINT_BIT); \
                 PUSHBUTTON ## n ## _PUE |= (1 << PUSHBUTTON ## n ## _BIT);
    FOR_X_FROM_1_TO_N_PUSHBUTTONS_DO(SETUP)
#undef SETUP
}

static volatile uint8_t button_pressed = 0;
static void process_button_press()
{
    uint8_t button_pressed = 0;
#define IF(n)                                                          \
    if (! (PUSHBUTTON ## n ## _PIN & (1 << PUSHBUTTON ## n ## _BIT)))  \
        button_pressed |= (1 << (n-1));
    FOR_X_FROM_1_TO_N_PUSHBUTTONS_DO(IF)
#undef IF

#if DEBUG
    // Decode for button arrangement on protoboard, buttons numbered 1-5
    // from left-right / top-bottom.
    tx_byte('P');
    if (button_pressed == 1) {
        tx_byte('1');
    }
    else if (button_pressed == 2) {
        tx_byte('2');
    }
    else if (button_pressed == 4) {
        tx_byte('3');
    }
    else if (button_pressed == 6) {
        tx_byte('4');
    }
    else if (button_pressed == 3) {
        tx_byte('5');
    }
    else {
        tx_byte('?');
    }
#endif
}

#define DEF_ISR(pcint_n)           \
    ISR(PCINT ## pcint_n ## _vect) \
    {                              \
        process_button_press();    \
    }
FOR_EACH_PUSHBUTTON_PCMSK_DO(DEF_ISR)
#undef DEF_ISR


static void handle_button_press(uint8_t but)
{
    if (but == BUTTON_MENU) {
        if (global_meter_state.ui_mode == UI_MODE_MAIN_MENU)
            global_meter_state.ui_mode = UI_MODE_READING;
        else
            global_meter_state.ui_mode = UI_MODE_MAIN_MENU;
    }
}

ISR(WDT_vect)
{
    ;
}

static void go_to_idle_mode()
{
    // Prescale to time out every 64ms.
    WDTCSR &= ~((1 << WDP3) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0));
    WDTCSR |= ((1 << WDP3) | (0 << WDP2) | (0 << WDP1) | (1 << WDP0));
    WDTCSR |= (1 << WDIE) | (1 << WDE);
    MCUCR &= ~((1 << SM1) | (1 << SM0));
}

int main()
{
#ifdef DEBUG
    tx_byte('I');
#endif

    setup_pushbuttons();
    setup_ADC();
    initialize_global_meter_states();

    setup_shift_register();
    set_shift_register_out();

    // Enable interrupts.
    sei();

    // Turn things on.
    or_shift_register_bits((1 << SHIFT_REGISTER_SCRPWR_BIT) |
                           (1 << SHIFT_REGISTER_OAPWR_BIT));
    set_shift_register_out();

    set_diode_configuration(3, DIODE_INCIDENT_NOND);

    display_init();
    display_clear();
    ui_show_interface();

    // The following code is useful for testing that all pins are connected.
    // It sets all pins to output mode and switches them from low to high
    // every half second.
    /*DDRA = 0xFF;
    DDRB = 0xFF;
    DDRC = 0xFF;
    PORTA |= 0xFF;
    PORTB |= 0xFF;
    PORTC |= 0xFF;
    for (;;) {
        PORTA ^= 0xFF;
        PORTB ^= 0xFF;
        PORTC ^= 0xFF;
        _delay_ms(500);
    }*/

    // The main loop. This looks at the latest exposure
    // reading every so often.
    uint8_t cnt;
    for (cnt = 0;; ++cnt) {
        if (button_pressed) {
            handle_button_press(button_pressed);
        }

        handle_measurement(get_adc_reading());
        global_transient_meter_state.exposure_ready = true;
        ui_show_interface();

        _delay_ms(50);
    }

    return 0;
}
