#ifndef I2C_H
#define I2C_H

#define I2C_READ  1
#define I2C_WRITE 0

void i2c_data_high(void);
void i2c_data_low(void);
void i2c_clock_high(void);
void i2c_clock_low(void);
void i2c_start(void);
void i2c_stop(void);
uint8_t i2c_read_byte(uint8_t nack);
uint8_t i2c_write_byte(uint8_t b);

#endif
