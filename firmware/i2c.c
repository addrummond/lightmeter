#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_i2c.h>
#include <stm32f0xx_rcc.h>

#include <deviceconfig.h>

// See ref manual 25.4.10
/*#define PRESC 1
#define SCLDEL 0x4
#define SDADEL 0x2
#define SCLH 0xC3
#define SCLL 0xC7
#define I2C_TIMING (PRESC << 28) | (SCLDEL << 20) | (SDADEL << 16) | (SCLH << 8) | (SCLL << 0)*/
// Taken from code for LM75.
#define I2C_TIMING 0x1045061D

void i2c_init()
{
    //
    // Copying from LM75_LowLevel_Init().
    //

    // Enable peripheral clock.
    RCC_APB1PeriphClockCmd(I2C_CLK, ENABLE);

    // Use HSI as clock source.
    RCC_I2CCLKConfig(RCC_I2C1CLK_HSI);

    RCC_AHBPeriphClockCmd(I2C_SCL_GPIO_CLK | I2C_SDA_GPIO_CLK, ENABLE);

    // Set alternate function.
    GPIO_PinAFConfig(I2C_SCL_GPIO_PORT, I2C_SCL_SOURCE, GPIO_AF_1);
    GPIO_PinAFConfig(I2C_SDA_GPIO_PORT, I2C_SDA_SOURCE, GPIO_AF_1);

    // ... skipping SMBUS thing in code we're copying from ...

    GPIO_InitTypeDef gpi;
    gpi.GPIO_Pin = I2C_SCL_PIN;
    gpi.GPIO_Mode = GPIO_Mode_AF;
    gpi.GPIO_Speed = GPIO_Speed_2MHz;
    gpi.GPIO_OType = GPIO_OType_OD;
    gpi.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(I2C_SCL_GPIO_PORT, &gpi);

    gpi.GPIO_Pin = I2C_SDA_PIN;
    GPIO_Init(I2C_SDA_GPIO_PORT, &gpi);

    //
    // Copying from LM75_Init()
    //
    I2C_InitTypeDef i2ci;
    I2C_StructInit(&i2ci);
    i2ci.I2C_Mode = I2C_Mode_I2C;
    i2ci.I2C_AnalogFilter = I2C_AnalogFilter_Enable;
    i2ci.I2C_DigitalFilter = 0x00;
    i2ci.I2C_OwnAddress1 = 0x00;
    i2ci.I2C_Ack = I2C_Ack_Enable;
    i2ci.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2ci.I2C_Timing = I2C_TIMING;

    I2C_Init(I2C_I2C, &i2ci);
    I2C_Cmd(I2C_I2C, ENABLE);
}
