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
*/

//
// Attiny84 final(ish) configuration.
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

#endif
