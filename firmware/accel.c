// See http://codinglab.blogspot.ca/2008/10/i2c-on-avr-using-bit-banging.html
// for info on I2C master bitbanging (and also Wikipedia page on I2C).

#include <deviceconfig.h>
#include <avr/io.h>
#include <util/delay.h>

#define STD_DELAY() (_delay_us(15))

static void data_hi()
{
    ACCEL_SDA_DDR &= ~(1 << ACCEL_SDA_BIT); // Set for input.
}

static void data_low()
{
    ACCEL_SDA_DDR |= (1 << ACCEL_SDA_BIT);
    ACCEL_SDA_PORT |= (1 << ACCEL_SDA_BIT);
}

static void clock_hi()
{
    ACCEL_SCL_DDR &= ~(1 << ACCEL_SDA_BIT); // Set for input.
}

static void clock_low()
{
    ACCEL_SCL_DDR |= (1 << ACCEL_SDA_BIT);
    ACCEL_SCL_PORT |= (1 << ACCEL_SDA_BIT);
}

static void i2c_start()
{
#if ACCEL_SCL_PORT != ACCEL_SDA_PORT
#error "foo"
#endif
    ACCEL_SDA_DDR |= ~((1 << ACCEL_SDA_BIT) | (1 << ACCEL_SCL_BIT));
    STD_DELAY();

    data_low();
    STD_DELAY();

    clock_low();
    STD_DELAY();
}

static void i2c_stop()
{
    data_low();
    STD_DELAY();
    // Clock stretching.
    uint8_t i;
    for (i = 0; i < 255 && (ACCEL_SCL_PIN >> ACCEL_SCL_BIT) & 1 == 0; ++i);
    STD_DELAY();
    data_high();
    STD_DELAY();
}

static uint8_t i2c_read_bit()
{
    data_high();
    STD_DELAY();
    // Clock stretching.
    uint8_t i = 0;
    for (i = 0; i < 255 && (ACCEL_SCL_PIN >> ACCEL_SCL_BIT) & 1 == 0; ++i);
    uint8_t bit = (ACCEL_SDA_PIN >> ACCEL_SDA_BIT) & 1;
    STD_DELAY();
    clock_low();
    return bit;
}

static void i2c_write_bit(uint8_t c)
{
    if (c == 0) {
        data_low();
    }
    else {
        data_high();
    }

    STD_DELAY();
    uint8_t i;
    for (i = 0; i < 255 && (ACCEL_SCN_PIN >> ACCEL_SCL_BIT) & 1 == 0; ++i);

    // According to Wiki pseudocode, we should check that no-one else is driving
    // SDA low at this point, but there's not much that we can do anyway in
    // this error condition.

    STD_DELAY();
    clock_low();
}

static void i2c_write_byte(uint8_t b)
{
    uint8_t nack;
    uint8_t i;
    for (i = 0; i < 8; ++i, b <<= 1)
        i2c_write_bit(byte >> 7);
    // Wiki pseudocode saves value of ACK bit and returns it,
    // but I can't see any use for it.
    i2c_read_bit();
}

static uint8_t i2c_read_byte(uint8_t nack)
{
    uint8_t r = 0;
    uint8_t i;
    for (i = 0; i < 8; ++i) {
        r <<= 1;
        r |= i2c_read_bit();
    }

    i2c_write_bit(nack);

    return r;
}

// See table 11 on p. 18 of datasheet for explanation of magic values.
#define WRITE 0x3A
#define READ  0x3B

static void i2c_read_register(uint8_t address, uint8_t regaddr)
{
    i2c_start();
    i2c_write_byte(WRITE);
    i2c_write_byte(regaddr);
    i2c_start();
    i2c_write_byte(READ);
    uint8_t val = i2c_read_byte(1);
    i2c_stop();
}

static void i2c_write_register(uint8_t regaddr, uint8_t value)
{
    i2c_start();
    i2c_write_byte(WRITE);
    i2c_write_byte(regaddr);
    i2c_write_byte(value);
    i2c_stop();
}

static void accel_setup()
{
    // Ensure that pullups are disabled on pins.
    ACCEL_SCL_PUE &= ~(1 << ACCEL_SCL_BIT);
    ACCEL_SDA_PUE &= ~(1 << ACCEL_SCL_BIT);

    // Set both pins to input mode.
    clock_high();
    data_high();
}

// Register addresses for accel IC (see table 12 on p. 20 of datasheet).
#define STATUS          0x00
#define OUT_X_MSB       0x01
#define OUT_X_LSB       0x02
#define OUT_Y_MSB       0x03
#define OUT_Y_LSB       0x04
#define OUT_Z_MSB       0x05
#define OUT_Z_LSB       0x06

#define SYSMOD          0x0B
#define INT_SOURCE      0x0C
#define WHO_AM_I        0x0D
#define XYZ_DATA_CFG    0x0E

#define PL_STATUS       0x10
#define PL_CFG          0x11
#define PL_COUNT        0x12
#define PL_BF_ZCOMP     0x12
#define PL_THS_REG      0x14
#define FF_MT_CFG       0x15
#define FF_MT_SRC       0x16
#define FF_MT_THS       0x17
#define FF_MT_COUNT     0x18

#define ASLP_COUNT      0x29
#define CTRL_REG1       0x2A
#define CTRL_REG2       0x2B
#define CTRL_REG3       0x2C
#define CTRL_REG4       0x2D
#define CTRL_REG5       0x2E
#define OFF_X           0x2F
#define OFF_Y           0x30
#define OFF_Z           0x31
