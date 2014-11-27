#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>

//
// Attiny1634 final(ish) configuration.
//


//
//  Currently we are leaving pins used for serial programming unattached to
//  make in-circuit programming more reliable. In principle some of these pins
//  could be re-used, but this seems to cause weird issues in practice, even
//  when the programmer is disconnected.
//
//          Display data |    ADC5/PB0 1---20 [MOSI]    | **********
//            Display DC |    ADC4/PA7 2---19 [MISO]    | **********
//           Display RST |    ADC3/PA6 3---18 PB3/ADC8  | Battery stat input
//     Serial debug port |    ADC2/PA5 4---17 PC0/ADC9  | Op amp output
//      Shift register A |    ADC1/PA4 5---16 [USCK]    | **********
//          Pushbutton 1 |    ADC0/PA3 6---15 PC2/ADC11 | Shift register clock
//          Pushbutton 2 |         PA2 7---14 [RESET]   | **********
//          Pushbutton 3 |         PA1 8---13 PC4       | Display CS
//           ADC ext ref |         PA0 9---12 PC5       | Display CLK
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
// Set ADC for PC0 input.
#define ADMUX_LIGHT_SOURCE             ((1 << MUX3) | (0 << MUX2) | (0 << MUX1) | (1 << MUX0))

#define ADMUX_TEMPERATURE_SOURCE       ((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0))

// Use 1.1V internal reference.
#define ADMUX_TEMP_REF_VOLTAGE         ((1 << REFS1) | (0 << REFS0))

// Use Vcc as reference voltage.
#define ADMUX_LIGHT_SOURCE_REF_VOLTAGE ((0 << REFS1) | (0 << REFS0))

// At some point we may want to switch to the internal 1.1V reference as a lame
// substitute for increasing gain on the attiny1634. This would in effect give
// us about 2.5x gain over measuring against VCC (2.8V when running on batteries).
#define ADMUX_LOW_LIGHT_REF_VOLTAGE    ((1 << REFS1) | (0 << REFS0))

#define ADC_LIGHT_DIDR                 DIDR1
#define ADC_LIGHT_DIDR_BIT             3


//
// Pushbutton ports configuration.
//
#define PUSHBUTTON1_PORT         PORTA
#define PUSHBUTTON1_PIN          PINA
#define PUSHBUTTON1_PUE          PUEA
#define PUSHBUTTON1_DDR          DDRA
#define PUSHBUTTON1_BIT          PA3
#define PUSHBUTTON1_PCMSK        PCMSK0
#define PUSHBUTTON1_PCIE         PCIE0
#define PUSHBUTTON1_PCINT_BIT    PCINT3
#define PUSHBUTTON1_PCINT_VECT   PCINT0_vect

#define PUSHBUTTON2_PORT         PORTA
#define PUSHBUTTON2_PIN          PINA
#define PUSHBUTTON2_PUE          PUEA
#define PUSHBUTTON2_DDR          DDRA
#define PUSHBUTTON2_BIT          PA2
#define PUSHBUTTON2_PCMSK        PCMSK0
#define PUSHBUTTON2_PCIE         PCIE0
#define PUSHBUTTON2_PCINT_BIT    PCINT2
#define PUSHBUTTON2_PCINT_VECT   PCINT0_vect

#define PUSHBUTTON3_PORT         PORTA
#define PUSHBUTTON3_PIN          PINA
#define PUSHBUTTON3_PUE          PUEA
#define PUSHBUTTON3_DDR          DDR
#define PUSHBUTTON3_BIT          PA1
#define PUSHBUTTON3_PCMSK        PCMSK0
#define PUSHBUTTON3_PCIE         PCIE0
#define PUSHBUTTON3_PCINT_BIT    PCINT1
#define PUSHBUTTON3_PCINT_VECT   PCINT1_vect

#define FOR_X_FROM_1_TO_N_PUSHBUTTONS_DO(x) x(1) x(2) x(3)
#define FOR_EACH_PUSHBUTTON_PCMSK_DO(x)     x(0) x(2)

#define BUTTON_METER  1
#define BUTTON_MENU   2
#define BUTTON_UP     4
#define BUTTON_DOWN   6


//
// Shift register outputs.
//
#define SHIFT_REGISTER_SCRPWR_BIT    0
#define SHIFT_REGISTER_STG1_BIT      2
#define SHIFT_REGISTER_STG2_BIT      3
#define SHIFT_REGISTER_OAPWR_BIT     4
#define SHIFT_REGISTER_DIODESW1_BIT  1
#define SHIFT_REGISTER_DIODESW2_BIT  7
#define SHIFT_REGISTER_DIODESW3_BIT  6
#define SHIFT_REGISTER_DIODESW4_BIT  5


//
// Port for sending serial debug output.
//
#define UART_PORT        PORTA
#define UART_PORT_NUMBER 5 // I.e. PA5


//
// Shift register output port.
//
#define SHIFT_REGISTER_OUTPUT_PORT  PORTA
#define SHIFT_REGISTER_OUTPUT_DDR   DDRA
#define SHIFT_REGISTER_OUTPUT_BIT   PA4
#define SHIFT_REGISTER_CLK_PORT     PORTC
#define SHIFT_REGISTER_CLK_DDR      DDRC
#define SHIFT_REGISTER_CLK_BIT      PC2


#endif
