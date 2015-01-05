#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_i2c.h>

void i2c_init()
{
    // Enable peripheral clock.
    RCC_APB1PeriphClockCmd(DISPLAY_I2C, ENABLE);

    // Use HSI as clock source.
    RCC_I2CCLKConfig(RCC_I2C1CLK_HSI);

    RCC_AHBPeriphClockCmd(I2C_SCL_PERIPH_PORT | I2C_SDA_PERIPH_PORT, ENABLE);

    GPIO_PinAFConfig(I2C_SCL_PORT, LM75_I2C_SCL_PIN_SOURCE, GPIO_AF_1);
    GPIO_PinAFConfig(I2C_SDA_PORT, LM75_I2C_SDA_PIN_SOURCE, GPIO_AF_1);

    GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(I2C_SCL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = I2C_SDA_PIN;
    GPIO_Init(LM75_I2C_SDA_GPIO_PORT, &GPIO_InitStructure);


    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_AnalogFilter = I2C_AnalogFilter_Disable;
    I2C_InitStructure.I2C_DigitalFilter = 0x00;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_Timing =
        // See ref manual 25.4.10
        // PRESC = 1
        // SCLL = 0xC7
        // SCLH = 0xC3
        // SDADEL = 0x2
        // SCLDEl = 0x4
        (PRESC << 28) | (SCLDEL << 20) | (SDADEL << 16) | (SCLH << 8) | (SCLL << 0);
}
