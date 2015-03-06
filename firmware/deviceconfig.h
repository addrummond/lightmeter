#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>

#include <stm32f0xx_gpio.h>
#include <stm32f0xx_i2c.h>

/*

STM32F030K6T6 configuration.

                  I  I
            V  B  2  2  P  P
            S  O  C  C  Z  Z  S  S
            S  O  S  S  P  N  T  T
               T  D  C  O  E  G  G
               0  A  L  S  H  1  2
            |  |  |  |  |  |  |  |
            ________________________
VDD      --|                       |-- SWDCLK/USBD+
           |                       |
         --|                       |-- SWDIO/USBD-
           |                       |
         --|                       |-- PBI$1
           |                       |
NRST     --|                       |-- PIEZOPOS
           |                       |
VDDA     --|                       |-- PIEZONEG
           |                       |
MEASURE1 --|                       |-- PBI$2
           |                       |
SCRPWR   --|                       |-- SCRRES
           |                       |
MEASURE2 --|                       |-- VDD
           |_______________________|
            |  |  |  |  |  |  |  |
            B  D  B  U  U  I     V
            R  I  A  S  S  N     S
            E  O  T  B  B  T     S
            S  D  V  D  D  E
            E  E  O  +  -  G
            T  S  L        C
               W  T        L
                  A        R
                  G
                  E
*/

//
// Pushbutton ports configuration.
//
#define PUSHBUTTON1_PORT     GPIOA
#define PUSHBUTTON1_PIN      GPIO_Pin_12

#define PUSHBUTTON2_PORT     GPIOA
#define PUSHBUTTON2_PIN      GPIO_Pin_9

#define FOR_X_FROM_1_TO_N_PUSHBUTTONS_WITH_GPIO_DO(x) x(1, GPIOA) x(2, GPIOA)


//
// Resistor/diode switches configuration.
//
#define DIODESW1_PORT        GPIOF
#define DIODESW1_PIN         GPIO_Pin_1

#define DIODESW2_PORT        GPIOA
#define DIODESW2_PIN         GPIO_Pin_2

#define DIODESW3_PORT        GPIOA
#define DIODESW3_PIN         GPIO_Pin_3

#define STG1_GPIO_PORT       GPIOB
#define STG1_PIN             GPIO_Pin_3

#define STG2_GPIO_PORT       GPIOA
#define STG2_PIN             GPIO_Pin_15


//
// ADC pins for light sensor measurement.
//

#define MEASURE1_GPIO_PORT   GPIOA
#define MEASURE1_PIN         GPIO_Pin_0
#define MEASURE1_ADC_CHANNEL ADC_Channel_0
#define MEASURE2_GPIO_PORT   GPIOA
#define MEASURE2_PIN         ADC_Channel_1
#define MEASURE2_ADC_CHANNEL ADC_IN2


//
// I2C
//

#define I2C_I2C              I2C1
#define I2C_CLK              RCC_APB1Periph_I2C1
#define I2C_SDA_GPIO_PORT    GPIOB
#define I2C_SDA_PIN          GPIO_Pin_7
#define I2C_SDA_SOURCE       GPIO_PinSource7
#define I2C_SCL_GPIO_PORT    GPIOB
#define I2C_SCL_PIN          GPIO_Pin_6
#define I2C_SCL_SOURCE       GPIO_PinSource6
#define I2C_SDA_GPIO_CLK     RCC_AHBPeriph_GPIOB
#define I2C_SCL_GPIO_CLK     RCC_AHBPeriph_GPIOB


//
// Screen
//

#define DISPLAY_RESET_GPIO_PORT  GPIOA
#define DISPLAY_RESET_PIN        GPIO_Pin_8

#endif
