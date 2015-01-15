#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>

#include <stm32f0xx_gpio.h>
#include <stm32f0xx_i2c.h>

/*

STM32F030K6T6 configuration.

            V  B  P  P
            S  O  B  B
            S  O  I  I
               T  2  1
               0
            |  |  |  |  |  |  |  |
            ________________________
VDD      --|                       |-- SWDCLK
           |                       |
DIODESW  --|                       |-- SWDIO
           |                       |
NRST     --|                       |-- USBDM
           |                       |
VDDA     --|                       |-- USBDP
           |                       |
OAOUT1   --|                       |-- I2CSDA
           |                       |
SCRPWR   --|                       |-- I2CSCL
           |                       |
OAOUT2   --|                       |-- SCRRES
           |_______________________|
            |  |  |  |  |  |  |  |
            B  D  B        I  F  V
            R  I  A        N  L  S
            E  O  T        T  A  S
            S  D  V        E  S
            E  E  O        G  H
            T  S  L        C  S
               W  T        L  Y
                  A        R  N
                  G           C
                  E
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
// I2C
//

#define I2C_I2C              I2C1
#define I2C_CLK              RCC_APB1Periph_I2C1
#define I2C_SDA_GPIO_PORT    GPIOA
#define I2C_SDA_PIN          GPIO_Pin_10
#define I2C_SDA_SOURCE       GPIO_PinSource10
#define I2C_SCL_GPIO_PORT    GPIOA
#define I2C_SCL_PIN          GPIO_Pin_9
#define I2C_SCL_SOURCE       GPIO_PinSource9
#define I2C_SDA_GPIO_CLK     RCC_AHBPeriph_GPIOB
#define I2C_SCL_GPIO_CLK     RCC_AHBPeriph_GPIOB

//
// Screen
//

#define SCREEN_RESET_PORT    GPIOA
#define SCREEN_RESET_PIN     GPIO_Pin_8

#endif
