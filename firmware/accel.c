#include <accel.h>
#include <i2c.h>
#include <deviceconfig.h>
#include <debugging.h>

#define ACCEL_I2C_ADDR (0x1D << 1)

// Single byte write
void accel_write_register(uint8_t reg, uint8_t byte)
{
    I2C_WAIT_ON_FLAG_NO_RESET(I2C_ISR_BUSY, "accel_write_register 1");
    I2C_TransferHandling(I2C_I2C, ACCEL_I2C_ADDR, 2, I2C_AutoEnd_Mode, I2C_Generate_Start_Write);
    I2C_WAIT_ON_FLAG_RESET(I2C_ISR_TXIS, "accel_write_register 2");
    I2C_SendData(I2C_I2C, reg);
    I2C_SendData(I2C_I2C, byte);
    I2C_WAIT_ON_FLAG_RESET(I2C_ISR_STOPF, "accel_write_register 3");
}

uint8_t accel_read_register(uint8_t reg)
{
    I2C_WAIT_ON_FLAG_NO_RESET_NORET(I2C_ISR_BUSY, "accel_read_register 1");
    I2C_TransferHandling(I2C_I2C, ACCEL_I2C_ADDR, 1, I2C_SoftEnd_Mode, I2C_Generate_Start_Write);
    I2C_WAIT_ON_FLAG_RESET_NORET(I2C_ISR_TXIS, "accel_read_register 2");
    I2C_SendData(I2C_I2C, reg);
    I2C_WAIT_ON_FLAG_RESET_NORET(I2C_ISR_TC, "accel_read_register 3");
    I2C_TransferHandling(I2C_I2C, ACCEL_I2C_ADDR, 1, I2C_AutoEnd_Mode, I2C_Generate_Start_Read);
    I2C_WAIT_ON_FLAG_RESET_NORET(I2C_ISR_RXNE, "accel_read_register 4");
    uint8_t v = I2C_ReceiveData(I2C_I2C);
    I2C_WAIT_ON_FLAG_RESET_NORET(I2C_ISR_STOPF, "accel_read_register 5");
    I2C_ClearFlag(I2C_I2C, I2C_ICR_STOPCF);
    return v;
}

void accel_init()
{
    // Wake the device by writing to ctrl 1 ACTIVE bit.
    // At the same time, we configure:
    //     ASLP_RATE (7-6)
    //     DR        (5-3)
    //     F_READ    (1)
    //     ACTIVE    (0)
    accel_write_register(ACCEL_REG_CTRL_REG1, 0b10000001);
    accel_write_register(ACCEL_REG_CTRL_REG2, 0b00000100);
    accel_write_register(ACCEL_REG_CTRL_REG3, 0b00101000);
    accel_write_register(ACCEL_REG_CTRL_REG4, 0b10000000);

    // Sleep timer is set in units of 1/320th of a millisecond (see p. 35 of datasheet).
    accel_write_register(ACCEL_REG_ASLP_COUNT, 4);
}
