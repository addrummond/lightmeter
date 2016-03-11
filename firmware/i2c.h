#ifndef I2C_H
#define I2C_H

#include <stm32f0xx_i2c.h>

void i2c_init(void);
void i2c_log_timeout(const char *msg, uint32_t length);

#define FLAG_TIMEOUT     ((uint32_t)0x1000)

#define I2C_WAIT_ON_FLAG_(flag, msg, op, ret, brk)                          \
    do {                                                                    \
        uint32_t to = FLAG_TIMEOUT;                                         \
        while (I2C_GetFlagStatus(I2C_I2C, (flag)) op RESET) {               \
            if (to-- == 0)                                                  \
            { ret i2c_log_timeout((msg), sizeof(msg)/sizeof(char)); brk; }  \
        }                                                                   \
    } while (0)
#define I2C_WAIT_ON_FLAG_NO_RESET(flag, msg)       I2C_WAIT_ON_FLAG_((flag), (msg), !=, return, )
#define I2C_WAIT_ON_FLAG_RESET(flag, msg)          I2C_WAIT_ON_FLAG_((flag), (msg), ==, return, )
#define I2C_WAIT_ON_FLAG_NO_RESET_NORET(flag, msg) I2C_WAIT_ON_FLAG_((flag), (msg), !=, ,break)
#define I2C_WAIT_ON_FLAG_RESET_NORET(flag, msg)    I2C_WAIT_ON_FLAG_((flag), (msg), ==, ,break)

#endif
