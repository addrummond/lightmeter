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
DIODESW  --|                       |-- SWDIO
           |                       |
NRST     --|                       |-- 
           |                       |
VDDA     --|                       |-- SCRPWR
           |                       |
OAOUT1   --|                       |-- I2CSDA
           |                       |
BATVOL   --|                       |-- I2CSCL
           |                       |
OAOUT2   --|                       |-- SCRRES
           |_______________________|
            |  |  |  |  |  |  |  |
            B  S  S  S  S  I  F  V
            R  P  P  P  P  N  L  S
            E  I  I  I  I  T  A  S
            S  N  S  M  M  E  S
            E  S  C  I  O  G  H
            T  S  K  S  S  C  S
                     O  I  L  Y
                           R  N
                              C
*/

//
// Pushbutton ports configuration.
//
#define PUSHBUTTON1_PORT     GPIOB
#define PUSHBUTTON1_PIN      GPIO_Pin_6

#define PUSHBUTTON2_PORT     GPIOB
#define PUSHBUTTON2_PIN      GPIO_Pin_7

#define FOR_X_FROM_1_TO_N_PUSHBUTTONS_WITH_GPIO_DO(x) x(1, GPIOB) x(2, GPIOB)


//
// Resistor/diode switches configuration.
//
#define DIODESW1_PORT        GPIOF
#define DIODESW1_PIN         GPIO_Pin_1

#define DIODESW2_PORT        GPIOA
#define DIODESW2_PIN         GPIO_Pin_2

#define DIODESW3_PORT        GPIOA
#define DIODESW3_PIN         GPIO_Pin_3

#define STG1_PORT            GPIOB
#define STG1_PIN             GPIO_Pin_0

#define STG2_POTY            GPIOB
#define STG2_PIN             GPIO_Pin_1


//
// I2c
//

#define I2C_I2C              RCC_APB1Periph_I2C1
#define I2C_SDA_PORT         GPIOA
#define I2C_SDA_PIN_SOURCE   GPIO_PinSource_10
#define I2C_SCL_PORT         GPIOA
#define I2C_SCL_PIN_SOURCE   GPIO_PinSource_9
#define I2C_SDA_PERIPH_PORT  RCC_AHBPeriph_GPIOB
#define I2C_SCL_PERIPH_PORT  RCC_AHBPeriph_GPIOB

#endif
