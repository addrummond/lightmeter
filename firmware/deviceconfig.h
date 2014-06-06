#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>

//
// Attiny1634 final(ish) configuration.
//


//
//  Currently we are leaving pins used for serial programming unattached to
//  make in-circuit programming more reliable. With some care some of these pins
//  could be re-used (e.g. if they are only used as output pins in the
//  lightmeter app).
//
//        Display data   |    ADC5/PB0 1---20 [MOSI]    | **********
//        Display DC     |    ADC4/PA7 2---19 [MISO]    | **********
//        Display reset  |    ADC3/PA6 3---18 PB3/ADC8  | Op amp output
//     Serial debug port |    ADC2/PA5 4---17 PC0/ADC9  | Shift register CLK
//     Shift register A  |    ADC1/PA4 5---16 [USCK]    | **********
//     Pushbutton input  |    ADC0/PA3 6---15 PC2/ADC11 | Screen power MOSFET
//     Charge pump clock |         PA2 7---14 [RESET]   | **********
//                       |         PA1 8---13 PC4       | Display CS
//              Test LED |         PA0 9---12 PC5       | Display CLK
//            ********** |       [GND] 10--11 [VCC]     | **********


//
// Display
//
#define DISPLAY_DATA_PORT   PORTB
#define DISPLAY_DATA_DDR    DDRB
#define DISPLAY_DATA_BIT    PB0

#define DISPLAY_CS_PORT     PORTC
#define DISPLAY_CS_DDR      DDRC
#define DISPLAY_CS_BIT      PC4

#define DISPLAY_CLK_PORT    PORTC
#define DISPLAY_CLK_DDR     DDRC
#define DISPLAY_CLK_BIT     PC5

#define DISPLAY_DC_PORT     PORTA
#define DISPLAY_DC_DDR      DDRA
#define DISPLAY_DC_BIT      PA7

#define DISPLAY_RESET_PORT  PORTA
#define DISPLAY_RESET_DDR   DDRA
#define DISPLAY_RESET_BIT   PA6


//
// ADC
//
// Set ADC for PB3 input.
#define ADMUX_LIGHT_SOURCE             ((1 << MUX3) | (0 << MUX2) | (0 << MUX1) | (0 << MUX0))

#define ADMUX_TEMPERATURE_SOURCE       ((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0))

// Use 1.1V internal reference.
#define ADMUX_TEMP_REF_VOLTAGE         ((1 << REFS1) | (0 << REFS0))

// Use Vcc as reference voltage.
#define ADMUX_LIGHT_SOURCE_REF_VOLTAGE ((0 << REFS1) | (0 << REFS0))

// At some point we may want to switch to the internal 1.1V reference as a lame
// substitute for increasing gain on the attiny1634. This would in effect give
// us about 2.5x gain over measuring against VCC (2.8V when running on batteries).
#define ADMUX_LOW_LIGH_REF_VOLTAGE     ((1 << REFS1) | (0 << REFS0))

#define ADMUX_CLEAR_SOURCE             (~((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0)));
#define ADMUX_CLEAR_REF_VOLTAGE        (~0b11000000); // Couldn't do this with macros without getting overflow warning for some reason.

#define ADC_LIGHT_DIDR                 DIDR1
#define ADC_LIGHT_DIDR_BIT             3


//
// Pushbutton input port.
//
#define PUSHBUTTON_PORT       PORTA
#define PUSHBUTTON_PIN        PINA
#define PUSHBUTTON_PUE        PUEA
#define PUSHBUTTON_DDR        DDRA
#define PUSHBUTTON_BIT        PA3
#define PUSHBUTTON_PCMSK      PCMSK0
#define PUSHBUTTON_PCINT      PCINT0
#define PUSHBUTTON_PCINT_VECT PCINT0_vect


//
// Charge pump clock output ports.
//
#define CHARGE_PUMP_CLOCK_PORT  PORTA
#define CHARGE_PUMP_CLOCK_DDR   DDRA
#define CHARGE_PUMP_CLOCK_BIT   PA2


//
// Port for sending serial debug output.
//
#define UART_PORT        PORTA
#define UART_PORT_NUMBER 5 // I.e. PA5

//
// Shift register output port. Note that the shift register shares a clock
// with the charge pump.
//
#define SHIFT_REGISTER_OUTPUT_PORT  PORTA
#define SHIFT_REGISTER_OUTPUT_DDR   DDRA
#define SHIFT_REGISTER_OUTPUT_BIT   PA4
//#define SHIFT_REGISTER_CLR_PORT     PORTC
//#define SHIFT_REGISTER_CLR_DDR      DDRC
//#define SHIFT_REGISTER_CLR_BIT      PC2
#define SHIFT_REGISTER_CLK_PORT     PORTC
#define SHIFT_REGISTER_CLK_DDR      DDRC
#define SHIFT_REGISTER_CLK_BIT      PC0


//
// Test LED
//
#define TEST_LED_PORT    PORTA
#define TEST_LED_DDR     DDRA
#define TEST_LED_BIT     PA0

#endif
