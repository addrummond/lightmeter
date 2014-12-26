// See http://codinglab.blogspot.ca/2008/10/i2c-on-avr-using-bit-banging.html
// for info on I2C master bitbanging (and also Wikipedia page on I2C).

#include <deviceconfig.h>
#include <i2c.h>
#include <accel.h>
#include <avr/io.h>
#include <deviceconfig.h>

uint8_t accel_read_register(uint8_t regaddr)
{
    i2c_start();
    i2c_write_byte(ACCEL_I2C_ADDRESS_LA | I2C_WRITE);
    i2c_write_byte(regaddr);
    i2c_start();
    i2c_write_byte(ACCEL_I2C_ADDRESS_LA | I2C_WRITE);
    uint8_t val = i2c_read_byte(1);
    i2c_stop();
    return val;
}

void accel_write_register(uint8_t regaddr, uint8_t value)
{
    i2c_start();
    i2c_write_byte(ACCEL_I2C_ADDRESS_LA | I2C_WRITE);
    i2c_write_byte(regaddr);
    i2c_write_byte(value);
    i2c_stop();
}

void accel_setup()
{
    // Ensure that pullups are disabled on pins.
    ACCEL_SCL_PUE &= ~(1 << ACCEL_SCL_BIT);
    ACCEL_SDA_PUE &= ~(1 << ACCEL_SCL_BIT);

    // Set both pins to input mode.
    i2c_clock_high();
    i2c_data_high();
}
