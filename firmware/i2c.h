#ifndef I2C_H
#define I2C_H

void i2c_init(void);

#define I2C_WAIT_ON_FLAG_(tof, flag, msg, op, ret, brk)     \
do {                                                        \
    uint32_t to = FLAG_TIMEOUT;                             \
    while (I2C_GetFlagStatus(I2C_I2C, (flag)) op RESET) {   \
        if (to-- == 0)                                      \
        { ret tof((msg), sizeof(msg)/sizeof(char)); brk; }  \
    }                                                       \
} while (0)
#define I2C_WAIT_ON_FLAG_NO_RESET(tof, flag, msg)       I2C_WAIT_ON_FLAG_(tof, (flag), (msg), !=, return, )
#define I2C_WAIT_ON_FLAG_RESET(tof, flag, msg)          I2C_WAIT_ON_FLAG_(tof, (flag), (msg), ==, return, )
#define I2C_WAIT_ON_FLAG_NO_RESET_NORET(tof, flag, msg) I2C_WAIT_ON_FLAG_(tof, (flag), (msg), !=, ,break)
#define I2C_WAIT_ON_FLAG_RESET_NORET(tof, flag, msg)    I2C_WAIT_ON_FLAG_(tof, (flag), (msg), ==, ,break)

#endif
