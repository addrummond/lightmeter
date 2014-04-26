#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

//
// Attiny84 final(ish) configuration.
//


//                       |    VCC 1--14  GND      |
//         Display data  |    PB0 2--13  PA/ADC0  | Op amp output.
//         Display CS    |    PB1 3--12  PA/ADC1  |
//                       |  RESET 4--11  PA/ADC2  |
//         Display Clk   |    PB2 5--10  PA/ADC3  |
//         Display DC    |    PA7 6---9  PA/ADC4  |
//         Display reset |    PA6 7---8  PA/ADC5  | Test LED.


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
// Set differential ADC with PA0 input.
#define ADMUX_LIGHT_SOURCE             ((0 << MUX5) | (0 << MUX4) | (0 << MUX3) | \
                                        (0 << MUX2) | (0 << MUX1) | (0 << MUX0))

#define ADMUX_TEMPERATURE_SOURCE       ((1 << MUX5) | (0 << MUX4) | (0 << MUX3) | \
                                        (0 << MUX2) | (1 << MUX1) | (0 << MUX0))

// Use 1.1V internal reference.
#define ADMUX_TEMP_REF_VOLTAGE         ((1 << REFS1) | (0 << REFS0))

// Use Vcc as reference voltage.
#define ADMUX_LIGHT_SOURCE_REF_VOLTAGE ((0 << REFS1) | (0 << REFS0))


//
// Test LED
//

#define TEST_LED_PORT    PORTA
#define TEST_LED_DDR     DDRA
#define TEST_LED_BIT     PA5

#endif
