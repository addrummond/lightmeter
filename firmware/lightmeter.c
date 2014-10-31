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
    and_shift_register_bits(~(1 << (SHIFT_REGISTER_STG1_BIT + global_transient_meter_state.op_amp_resistor_stage - 1)));
    or_shift_register_bits(1 << (SHIFT_REGISTER_STG1_BIT + op_amp_resistor_stage - 1));
    set_shift_register_out();
    global_transient_meter_state.op_amp_resistor_stage = op_amp_resistor_stage;
}

static void turn_on_display()
{
    or_shift_register_bits(1 << SHIFT_REGISTER_SCRPWR_BIT);
    set_shift_register_out();
}

static void led_test(void);
void handle_measurement()
{
    // Copy the volatile value, which could change in the middle of this function.
    // Div by 4 because we're going from units of 1/1024 to units of 1/256.
    // (Could left adjust everything, but we might want the full 10 bits for temps.)
    uint8_t adc_light_nonvol_value = (uint8_t)(adc_light_value >> 2);
#ifdef DEBUG
    tx_byte('S');
    tx_byte(global_transient_meter_state.op_amp_resistor_stage);
    tx_byte('V');
    tx_byte(adc_light_nonvol_value);
#endif

    global_transient_meter_state.last_ev_with_fracs = get_ev100_at_temperature_voltage(
        current_temp,
        adc_light_nonvol_value,
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
    if (adc_light_nonvol_value > 250 && global_transient_meter_state.op_amp_resistor_stage < NUM_OP_AMP_RESISTOR_STAGES) {
        newr = global_transient_meter_state.op_amp_resistor_stage + 1;
    }
    else if (adc_light_nonvol_value <= VOLTAGE_TO_EV_ABS_OFFSET + 5 && global_transient_meter_state.op_amp_resistor_stage > 1) {
        newr = global_transient_meter_state.op_amp_resistor_stage - 1;
    }

    if (newr) {
        if (debounce > 0) {
            --debounce;
        }
        else {
            set_op_amp_resistor_stage(newr);
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
    button_pressed = 0;
#define IF(n)                                                          \
    if (! (PUSHBUTTON ## n ## _PIN & (1 << PUSHBUTTON ## n ## _BIT)))  \
        button_pressed |= (1 << (n-1));
    FOR_X_FROM_1_TO_N_PUSHBUTTONS_DO(IF)
#undef IF

#if DEBUG
    tx_byte('P');
    if (button_pressed < 8)
        tx_byte('A' + button_pressed - 1);
    else
        tx_byte('?');
#endif
}

#define DEF_ISR(pcint_n)           \
    ISR(PCINT ## pcint_n ## _vect) \
    {                              \
        process_button_press();    \
    }
FOR_EACH_PUSHBUTTON_PCMSK_DO(DEF_ISR)
#undef DEF_ISR

static void setup_charge_pump()
{
    DDRA |= (1 << PA6);
    DDRB |= (1 << PB3);

    // Enable fast 8-bit PWM mode with TOP=0xFF.
    TCCR1B = ((0 << WGM13) | (1 << WGM12));
    TCCR1A = ((0 << WGM11) | (1 << WGM10));

    // Set output pins out of phase.
    TCCR1A |= (1 << COM1A1);
    TCCR1A &= ~(1 << COM1A0);
    TCCR1A |= (1 << COM1B1) | (1 << COM1B0);

    // Prescale clock by 1/1.
    TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);
    OCR1A = 128;
    OCR1B = 128;
}

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

    setup_charge_pump();
    setup_shift_register();
    set_shift_register_out();

    sei();

    turn_on_display();
    display_init();
    display_clear();

    // The main loop. This looks at the latest exposure
    // reading every so often.
    uint8_t cnt;
    for (cnt = 0;; ++cnt) {
        if (button_pressed) {
            handle_button_press(button_pressed);
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

//#ifdef DEBUG
//    tx_byte('X');
//#endif

        // Put device in idle mode for a bit.
        //go_to_idle_mode();
        _delay_ms(50);
    }

    return 0;
}
