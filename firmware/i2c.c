#include <avr/io.h>
#include <util/delay.h>
#include <deviceconfig.h>

#define STD_DELAY() (_delay_us(15))

void i2c_data_high()
{
    ACCEL_SDA_DDR &= ~(1 << ACCEL_SDA_BIT); // Set for input.
}

void i2c_data_low()
{
    ACCEL_SDA_DDR |= (1 << ACCEL_SDA_BIT);
    ACCEL_SDA_PORT |= (1 << ACCEL_SDA_BIT);
}

void i2c_clock_high()
{
    ACCEL_SCL_DDR &= ~(1 << ACCEL_SDA_BIT); // Set for input.
}

void i2c_clock_low()
{
    ACCEL_SCL_DDR |= (1 << ACCEL_SDA_BIT);
    ACCEL_SCL_PORT |= (1 << ACCEL_SDA_BIT);
}

void i2c_start()
{
//#if ACCEL_SCL_PORT != ACCEL_SDA_PORT
//#error "Code in i2c_start() in i2c.c assumes that ACCEL_SCL_PORT == ACCEL_SDA_PORT"
//#endif
    ACCEL_SDA_DDR |= ~((1 << ACCEL_SDA_BIT) | (1 << ACCEL_SCL_BIT));
    STD_DELAY();

    i2c_data_low();
    STD_DELAY();

    i2c_clock_low();
    STD_DELAY();
}

void i2c_stop()
{
    i2c_data_low();
    STD_DELAY();
    // Clock stretching.
    uint8_t i;
    for (i = 0; i < 255 && ((ACCEL_SCL_PIN >> ACCEL_SCL_BIT) & 1) == 0; ++i);
    STD_DELAY();
    i2c_data_high();
    STD_DELAY();
}

static uint8_t i2c_read_bit()
{
    i2c_data_high();
    STD_DELAY();
    // Clock stretching.
    uint8_t i = 0;
    for (i = 0; i < 255 && ((ACCEL_SCL_PIN >> ACCEL_SCL_BIT) & 1) == 0; ++i);
    uint8_t bit = (ACCEL_SDA_PIN >> ACCEL_SDA_BIT) & 1;
    STD_DELAY();
    i2c_clock_low();
    return bit;
}

static void i2c_write_bit(uint8_t c)
{
    if (c == 0) {
        i2c_data_low();
    }
    else {
        i2c_data_high();
    }

    STD_DELAY();
    uint8_t i;
    for (i = 0; i < 255 && ((ACCEL_SCL_PIN >> ACCEL_SCL_BIT) & 1) == 0; ++i);

    // According to Wiki pseudocode, we should check that no-one else is driving
    // SDA low at this point, but there's not much that we can do anyway in
    // this error condition.

    STD_DELAY();
    i2c_clock_low();
}

void i2c_write_byte(uint8_t b)
{
    uint8_t i;
    for (i = 0; i < 8; ++i, b <<= 1)
        i2c_write_bit(b >> 7);
    // Wiki pseudocode saves value of ACK bit and returns it,
    // but I can't see any use for it.
    i2c_read_bit();
}

uint8_t i2c_read_byte(uint8_t nack)
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
