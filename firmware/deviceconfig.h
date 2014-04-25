#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

//
// Attiny85 test code.
//
/*
#define DISPLAY_DATA_PORT PORTB
#define DISPLAY_DATA_DDR    DDRB
#define DISPLAY_DATA_BIT    PB0

#define DISPLAY_CS_PORT     PORTB
#define DISPLAY_CS_DDR      DDRB
#define DISPLAY_CS_BIT      PB1

#define DISPLAY_CLK_PORT    PORTB
#define DISPLAY_CLK_DDR     DDRB
#define DISPLAY_CLK_BIT     PB2

#define DISPLAY_DC_PORT     PORTB
#define DISPLAY_DC_DDR      DDRB
#define DISPLAY_DC_BIT      PB4

#define DISPLAY_RESET_PORT  PORTB
#define DISPLAY_RESET_DDR   DDRB
#define DISPLAY_RESET_BIT   PB3

*/

//
// Attiny84 final(ish) configuration.
//

//                       |    VCC 1--14  GND      |
//         Display data  |    PB0 2--13  PA/ADC0  |
//         Display CS    |    PB1 3--12  PA/ADC1  |
//                       |  RESET 4--11  PA/ADC2  |
//         Display Clk   |    PB2 5--10  PA/ADC3  |
//         Display DC    |    PA7 6---9  PA/ADC4  |
//                       |    PA6 7---8  PA/ADC5  |

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

#endif
