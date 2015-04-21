#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>

#include <stm32f0xx_gpio.h>
#include <stm32f0xx_i2c.h>
#include <stm32f0xx_syscfg.h>

/*

STM32F030K6T6 configuration.

                  I  I  L
               B  2  2  R
               O  C  C  G     S  S
            V  O  S  S  E     T  T
            S  T  D  C  B     G  G
            S  0  A  L  L     1  2
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
            D  M  B  P  P  I     V
            I  I  A  I  I  N     S
            O  C  T  E  E  T     S
            D  O  V  Z  Z  E
            E  U  O  O  O  G
            S  T  L  P  N  C
            W     T  O  E  L
                  A  S  G  R
                  G
                  E
*/

//
// USB
//
#define USB_DPLUS_GPIO_PORT          GPIOA
#define USB_DPLUS_PIN                GPIO_Pin_14
#define USB_DMINUS_GPIO_PORT         GPIOA
#define USB_DMINUS_PIN               GPIO_Pin_13


//
// Pushbutton ports configuration.
//
#define PUSHBUTTON1_GPIO_PORT        GPIOA
#define PUSHBUTTON1_GPIO_PORT_SOURCE EXTI_PortSourceGPIOA
#define PUSHBUTTON1_PIN              GPIO_Pin_12
#define PUSHBUTTON1_PIN_SOURCE       GPIO_PinSource12
#define PUSHBUTTON1_EXTI             EXTI12
#define PUSHBUTTON1_EXTI_LINE        EXTI_Line12

#define PUSHBUTTON2_GPIO_PORT        GPIOA
#define PUSHBUTTON2_GPIO_PORT_SOURCE EXTI_PortSourceGPIOA
#define PUSHBUTTON2_PIN              GPIO_Pin_9
#define PUSHBUTTON2_PIN_SOURCE       GPIO_PinSource9
#define PUSHBUTTON2_EXTI             EXTI9
#define PUSHBUTTON2_EXTI_LINE        EXTI_Line9


//
// Resistor/diode switches configuration.
//
#define DIODESW_PORT        GPIOA
#define DIODESW_PIN         GPIO_Pin_3


//
// ADC pins for light sensor measurement.
//

#define MEASURE1_GPIO_PORT   GPIOA
#define MEASURE1_PIN         GPIO_Pin_0
#define MEASURE1_ADC_CHANNEL ADC_Channel_0
#define MEASURE2_GPIO_PORT   GPIOA
#define MEASURE2_PIN         GPIO_Pin_2
#define MEASURE2_ADC_CHANNEL ADC_Channel_2
#define DIODESW_GPIO_PORT    GPIOA
#define DIODESW_PIN          GPIO_Pin_3
#define INTEGCLR_GPIO_PORT   GPIOB
#define INTEGCLR_PIN         GPIO_Pin_0
#define GB1_GPIO_PORT        GPIOB
#define GB1_PIN              GPIO_Pin_3
#define GB2_GPIO_PORT        GPIOA
#define GB2_PIN              GPIO_Pin_15


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
#define DISPLAY_POWER_GPIO_PORT  GPIOA
#define DISPLAY_POWER_PIN        GPIO_Pin_1

#endif
