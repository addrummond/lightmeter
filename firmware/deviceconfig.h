#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>

//
// Attiny1634 final(ish) configuration.
//


//
//        Display data   |    ADC5/PB0 1--20 PB1/ADC6  | Display CS
//        Display DC     |    ADC4/PA7 2--19 PB2/ADC7  | Display Clk
//        Display reset  |    ADC3/PA6 3--18 PB3/ADC8  | Op amp output
//     Serial debug port |    ADC2/PA5 4--17 PC0/ADC9  |
//     Pushbutton input  |    ADC1/PA4 5--16 PC1/ADC10 |
//     Charge pump clk 2 |    ADC0/PA3 6--15 PC2/ADC11 |
//     Charge pump clk 1 |         PA2 7--14 RESET     |
//                       |         PA1 8--13 PC4       |
//                       |         PA0 9--12 PC5       |
//                       |         GND 10--11 VCC      |


//
// Display
//
#define DISPLAY_DATA_PORT   PORTB
#define DISPLAY_DATA_DDR    DDRB
#define DISPLAY_DATA_BIT    PB0

#define DISPLAY_CS_PORT     PORTB
#define DISPLAY_CS_DDR      DDRB
#define DISPLAY_CS_BIT      PB1

#define DISPLAY_CLK_PORT    PORTB
#define DISPLAY_CLK_DDR     DDRB
#define DISPLAY_CLK_BIT     PB2

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


//
// Charge pump clock output ports.
//
#define CHARGE_PUMP_CLOCKS_PORT  PORTA

#define CHARGE_PUMP_CLOCK1_DDR   DDRA
#define CHARGE_PUMP_CLOCK1_BIT   PA2

#define CHARGE_PUMP_CLOCK2_DDR   DDRA
#define CHARGE_PUMP_CLOCK2_BIT   PA3


//
// Pushbutton input port.
//
#define PUSHBUTTON_PORT PORTA
#define PUSHBUTTON_PIN  PINA
#define PUSHBUTTON_DDR  DDRA
#define PUSHBUTTON_BIT  PA4

#define PUSHBUTTON_CAPVAL_TENTHS_MU_F   1L
#define PUSHBUTTON1_RVAL_KO             33L
#define PUSHBUTTON2_RVAL_KO             55L
#define PUSHBUTTON3_RVAL_KO             82L
#define PUSHBUTTON4_RVAL_KO             180L

#define PUSHBUTTON_RC_MS(n) ((uint8_t)((PUSHBUTTON ## n ## _RVAL_KO * 1000L * PUSHBUTTON_CAPVAL_TENTHS_MU_F * 1000L) / 10000000L))


//
// Port for sending serial debug output.
//
#define UART_PORT        PORTA
#define UART_PORT_NUMBER 5 // I.e. PA5


//
// Test LED
//
#define TEST_LED_PORT    PORTA
#define TEST_LED_DDR     DDRA
#define TEST_LED_BIT     PA5

#endif
