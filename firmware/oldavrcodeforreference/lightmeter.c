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

void setup_ADC()
{
    // Disable analogue comparator (unused).
    ACSRA |= (1 << ACD);

    ADMUX = (0 << REFS1) | (0 << REFS0);
    // Set ADC source to light sensor.
    ADMUX |= ADMUX_LIGHT_SOURCE | ADMUX_LIGHT_SOURCE_REF_VOLTAGE;

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

#define REFLECTIVE 0
#define INCIDENT 1
// Layout of diodes from left to right is:
//
//     incident without neutral density filter (NDF)
//     incident with NDF
//     refl with NDF
//     refl without NDF
//
static void set_amp_stage(uint8_t reflective_or_incident, uint8_t stage)
{
#define s(i)   ((STAGE ## i ## _STOPS_SUBTRACTED == 0 ? 0 : 1) << ((i)-1)) |
#define ss(i)  ((STAGE ## i ## _RESISTOR_NUMBER) << (((i)-1)*2)) |
    static const uint8_t  stage_uses_nd_filter = FOR_EACH_AMP_STAGE(s) 0;
    static const uint16_t stage_uses_resistor  = FOR_EACH_AMP_STAGE(ss) 0;
#undef s
#undef ss

    // Select appropriate ADC ref voltage.
    ADMUX &= ~((1 << REFS0) | (1 << REFS1));
    ADMUX |= ADMUX_LIGHT_SOURCE_REF_VOLTAGE;

    // Deselect all diodes.
    and_shift_register_bits(~((1 << SHIFT_REGISTER_DIODESW1_BIT) | (1 << SHIFT_REGISTER_DIODESW2_BIT) |
                              (1 << SHIFT_REGISTER_DIODESW3_BIT) | (1 << SHIFT_REGISTER_DIODESW4_BIT)));

    uint8_t undf = stage_uses_nd_filter & (1 << (stage-1));

    // Which diode do we use?
    uint8_t dbit;
    if (reflective_or_incident == REFLECTIVE) {
        if (undf) {
            dbit = SHIFT_REGISTER_DIODESW3_BIT;
        }
        else {
            dbit = SHIFT_REGISTER_DIODESW4_BIT;
        }
    }
    else {
        if (undf) {
            dbit = SHIFT_REGISTER_DIODESW2_BIT;
        }
        else {
            dbit = SHIFT_REGISTER_DIODESW1_BIT;
        }
    }
    or_shift_register_bits(1 << dbit);

    // Set all stage switch pins to zero.
    and_shift_register_bits(~((1 << SHIFT_REGISTER_STG1_BIT) | (1 << SHIFT_REGISTER_STG2_BIT)));

    // Which resistor do we use? (count starts from 0)
    uint8_t rnum = (stage_uses_resistor >> ((stage-1)*2)) & 0b11;
    if (rnum == 0) {
        or_shift_register_bits((1 << SHIFT_REGISTER_STG1_BIT) | (1 << SHIFT_REGISTER_STG2_BIT));
    }
    else if (rnum == 1) {
        or_shift_register_bits(1 << SHIFT_REGISTER_STG2_BIT);
    }
    else if (rnum == 2) {
        or_shift_register_bits(1 << SHIFT_REGISTER_STG1_BIT);
    }
    else { //if (rnum == 3) {
        ;
    }

    set_shift_register_out();
    global_transient_meter_state.op_amp_resistor_stage = stage;
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
    uint8_t light_value8 = (adc_light_value >> 2);

//#ifdef DEBUG
//    tx_byte('V');
//    tx_byte(light_value8);
//#endif

    global_transient_meter_state.last_ev_with_fracs = get_ev100_at_voltage(
        light_value8,
        global_transient_meter_state.op_amp_resistor_stage
    );

//#ifdef DEBUG
//    tx_byte('E');
//    tx_byte(ev_with_fracs_get_ev8(global_transient_meter_state.last_ev_with_fracs));
//#endif

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
    if (light_value8 > 250 && global_transient_meter_state.op_amp_resistor_stage < NUM_AMP_STAGES) {
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
            set_amp_stage(INCIDENT/*temporary default setting*/,newr);
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

    // Set up the 16-bit counter, which we use for debouncing the buttons.
    // Clock is scaled by /1024.
    TCCR1B |= (1 << CS02) | (0 << CS01) | (1 << CS00);
    // Turn on CTC.
    TCCR1B |= (0 << WGM13) | (1 << WGM12);
    TCCR1A |= (0 << WGM11) | (0 << WGM10);
    // This resets the counter a few times per second.
    OCR1A = 800;
    TIMSK |= (1 << OCIE1A);
}

static uint8_t debounced = 1;
ISR(TIM1_COMPA_vect)
{
    debounced = 1;
}

static uint8_t button_bounced()
{
    TCNT1 = 0;
    if (! debounced) {
        return 1;
    }
    else {
        debounced = 0;
        return 0;
    }
}

static uint8_t button_pressed = 0;
static void process_button_press()
{
    if (button_bounced())
        return;

    uint8_t button_pressed = 0;
#define IF(n)                                                       \
    if ((PUSHBUTTON ## n ## _PIN & (1 << PUSHBUTTON ## n ## _BIT))) \
        button_pressed |= (1 << (n-1));
    FOR_X_FROM_1_TO_N_PUSHBUTTONS_DO(IF)
#undef IF

    // Ignore this event if it's just the release of the previously-pushed button.
    if (button_pressed == 0 || button_pressed == 7)
        return;

#if DEBUG
    // Decode for button arrangement on protoboard, buttons numbered 1-3.
    // from left-right / top-bottom.
    tx_byte('P');
    if (button_pressed == 1) {
        tx_byte('1');
    }
    else if (button_pressed == 3) {
        tx_byte('2');

    }
    else if (button_pressed == 2) {
        tx_byte('3');
    }
    else if (button_pressed == 4) {
        tx_byte('5');
    }
    else {
        tx_byte('?');
        tx_byte('0' + button_pressed);
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
    or_shift_register_bits(1 << SHIFT_REGISTER_OAPWR_BIT);
    // Screen power MOSFET is p-channel with pullup.
    and_shift_register_bits(~(1 << SHIFT_REGISTER_SCRPWR_BIT));
    set_shift_register_out();

    set_amp_stage(INCIDENT, 2);

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

#ifdef DEBUG
    //tx_byte('L');
#endif
    }

    return 0;
}