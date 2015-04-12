#ifndef ACCEL_H
#define ACCEL_H

#include <stdint.h>

uint8_t accel_read_register(uint8_t regaddr);
void accel_write_register(uint8_t regaddr, uint8_t value);
void accel_setup(void);

#define ACCEL_I2C_ADDRESS    (0x1D << 1)

// Register addresses for accel IC (see table 12 on p. 20 of datasheet).
#define ACCEL_REG_STATUS          0x00
#define ACCEL_REG_OUT_X_MSB       0x01
#define ACCEL_REG_OUT_X_LSB       0x02
#define ACCEL_REG_OUT_Y_MSB       0x03
#define ACCEL_REG_OUT_Y_LSB       0x04
#define ACCEL_REG_OUT_Z_MSB       0x05
#define ACCEL_REG_OUT_Z_LSB       0x06

#define ACCEL_REG_SYSMOD          0x0B
#define ACCEL_REG_INT_SOURCE      0x0C
#define ACCEL_REG_WHO_AM_I        0x0D
#define ACCEL_REG_XYZ_DATA_CFG    0x0E

#define ACCEL_REG_PL_STATUS       0x10
#define ACCEL_REG_PL_CFG          0x11
#define ACCEL_REG_PL_COUNT        0x12
#define ACCEL_REG_PL_BF_ZCOMP     0x12
#define ACCEL_REG_PL_THS_REG      0x14
#define ACCEL_REG_FF_MT_CFG       0x15
#define ACCEL_REG_FF_MT_SRC       0x16
#define ACCEL_REG_FF_MT_THS       0x17
#define ACCEL_REG_FF_MT_COUNT     0x18

#define ACCEL_REG_ASLP_COUNT      0x29
#define ACCEL_REG_CTRL_REG1       0x2A
#define ACCEL_REG_CTRL_REG2       0x2B
#define ACCEL_REG_CTRL_REG3       0x2C
#define ACCEL_REG_CTRL_REG4       0x2D
#define ACCEL_REG_CTRL_REG5       0x2E
#define ACCEL_REG_OFF_X           0x2F
#define ACCEL_REG_OFF_Y           0x30
#define ACCEL_REG_OFF_Z           0x31

#endif
