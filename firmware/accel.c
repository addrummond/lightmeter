// See http://codinglab.blogspot.ca/2008/10/i2c-on-avr-using-bit-banging.html
// for info on I2C master bitbanging.

#include <deviceconfig.h>

#define STD_DELAY() (_delay(1))

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
    ACCEL_SDA_DDR |= ~((1 << ACCEL_SDA_BIT) | (1 << ACCEL_CLK_BIT));
    STD_DELAY();

    data_low();
    STD_DELAY();

    data_high();
    STD_DELAY();
}

static void i2c_stop()
{
    clock_high();
    STD_DELAY();

    data_high();
    STD_DELAY(1);
}

static uint8_t i2c_read_bit()
{
    data_high();
    clock_high();
    STD_DELAY();

    uint8_t c = (ACCEL_SDA_PIN >> ACCEL_SDA_BIT) & 1; // TODO CHECK LOGIC
    clock_low();
    STD_DELAY();

    return c;
}

static void i2c_write_bit(uint8_t c)
{
    if (c == 0) {
        data_low();
    }
    else {
        data_high();
    }

    clock_high();
    STD_DELAY();

    clock_low();
    STD_DELAY();

    data_low();
    STD_DELAY();
}

static void i2c_write_byte(uint8_t b)
{
    uint8_t i;
    for (i = 0; i < 8; ++i) {
        i2c_write_bit(c >> 7);
        c <<= 1;
    }
}

static uint8t i2c_read_byte(uint8_t ack)
{
    uint8_t r = 0;

    uint8_t i;
    for (i = 0; i < 8; ++i) {
        r <<= 1;
        r |= i2c_read_bit();
    }

    i2c_write_bit(!ack);

    STD_DELAY();

    return r;
}

void accel_setup()
{
    // Ensure that pullups are disabled on pins.
    ACCEL_SCL_PUE &= ~(1 << ACCEL_SCL_BIT);
    ACCEL_SDA_PUE &= ~(1 << ACCEL_SCL_BIT);

    clock_low();
    data_low();

    clock_high();
    data_high();

    STD_DELAY();
}
