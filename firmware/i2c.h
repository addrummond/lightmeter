#ifndef I2C_H
#define I2C_H

#define I2C_READ  1
#define I2C_WRITE 0

// Making these macros instead of functions significanly reduced code size.
#define i2c_data_high() (ACCEL_SDA_DDR &= ~(1 << ACCEL_SDA_BIT)) // Set for input.
#define i2c_data_low() ((ACCEL_SDA_DDR |= (1 << ACCEL_SDA_BIT)), (ACCEL_SDA_PORT |= (1 << ACCEL_SDA_BIT)))
#define i2c_clock_high() (ACCEL_SCL_DDR &= ~(1 << ACCEL_SDA_BIT)) // Set for input.
#define i2c_clock_low() ((ACCEL_SCL_DDR |= (1 << ACCEL_SDA_BIT)), (ACCEL_SCL_PORT |= (1 << ACCEL_SDA_BIT)))

void i2c_start(void);
void i2c_stop(void);
uint8_t i2c_read_byte(uint8_t nack);
void i2c_write_byte(uint8_t b);

#endif
