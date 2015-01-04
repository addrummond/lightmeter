#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>

/*

STM32F030K6T6 configuration.

            V  B  P  P  S  S  S  S
            S  O  B  B  P  P  P  P
            S  O  I  I  I  I  I  I
               T  2  1  M  M  S  N
               0        O  I  C  S
                        S  S  K  S
                        I  O

            |  |  |  |  |  |  |  |
            ________________________
VDD      --|                       |-- SWDCLK
           |                       |
DIODESW1 --|                       |-- SWDIO/BRESET
           |                       |
NRST     --|                       |-- OAPWR
           |                       |
VDDA     --|                       |-- SCRPWR
           |                       |
OAOUT2   --|                       |-- I2CSDA
           |                       |
BATVOL   --|                       |-- I2CSCL
           |                       |
DIODESW2 --|                       |-- SCRRES
           |_______________________|
            |  |  |  |  |  |  |  |
            D  S  S  S  S  S  S  V
            I  P  P  P  P  T  T  S
            O  I  I  I  I  G  G  S
            D  N  S  M  M  $  $
            E  S  C  I  O  1  2
            S  S  K  S  S
            W        O  I
            3
*/

//
// Pushbutton ports configuration.
//
#define PUSHBUTTON1_GPIO     GPIOB
#define PUSHBUTTON1_PIN      GPIO_Pin_6

#define PUSHBUTTON2_GPIO     GPIOB
#define PUSHBUTTON2_PIN      GPIO_Pin_7

#define FOR_X_FROM_1_TO_N_PUSHBUTTONS_DO(x) x(1) x(2)
#define FOR_EACH_PUSHBUTTON_PCMSK_DO(x)     x(0)


//
// Resistor/diode switches configuration.
//
#define DIODESW1_GPIO        GPIOF
#define DIODESW1_PIN         GPIO_Pin_1

#define DIODESW2_GPIO        GPIOA
#define DIODESW2_PIN         GPIO_Pin_2

#define DIODESW3_GPIO        GPIOA
#define DIODESW3_PIN         GPIO_Pin_3

#define STG1_GPIO            GPIOB
#define STG1_PIN             GPIO_Pin_0

#define STG2_GPIO            GPIOB
#define STG2_PIN             GPIO_Pin_1


//
// Display
//
#define DISPLAY_DATA_PORT   PORTB
#define DISPLAY_DATA_DDR    DDRB
#define DISPLAY_DATA_BIT    PB0

#define DISPLAY_CS_PORT     PORTC
#define DISPLAY_CS_DDR      DDRC
#define DISPLAY_CS_BIT      PC4

#define DISPLAY_CLK_PORT    PORTB
#define DISPLAY_CLK_DDR     DDRB
#define DISPLAY_CLK_BIT     PB3

#define DISPLAY_DC_PORT     PORTA
#define DISPLAY_DC_DDR      DDRA
#define DISPLAY_DC_BIT      PA7

#define DISPLAY_RESET_PORT  PORTA
#define DISPLAY_RESET_DDR   DDRA
#define DISPLAY_RESET_BIT   PA6


//
// Accelerometer.
//
#define ACCEL_SCL_PORT PORTA
#define ACCEL_SCL_DDR  DDRA
#define ACCEL_SCL_BIT  PA3
#define ACCEL_SCL_PUE  PUEA
#define ACCEL_SCL_PIN  PINA

#define ACCEL_SDA_PORT PORTA
#define ACCEL_SDA_DDR  DDRA
#define ACCEL_SDA_BIT  PA0
#define ACCEL_SDA_PUE  PUEA
#define ACCEL_SDA_PIN  PINA


//
// ADC
//
// Set ADC for PC0 input.
#define ADMUX_LIGHT_SOURCE             ((1 << MUX3) | (0 << MUX2) | (0 << MUX1) | (1 << MUX0))

// Use Vcc as reference voltage.
#define ADMUX_LIGHT_SOURCE_REF_VOLTAGE ((0 << REFS1) | (0 << REFS0))

#define ADC_LIGHT_DIDR                 DIDR1
#define ADC_LIGHT_DIDR_BIT             3


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
