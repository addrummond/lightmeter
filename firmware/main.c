#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_rcc.h>

#include <i2c.h>
#include <display.h>
#include <debugging.h>

int main()
{
    debugging_writec("Hello World!\n");

    i2c_init();
    //display_init();

    //display_reset();
    //const uint8_t page_array[] = { 0xFF, 0xFF };
    //display_write_page_array(page_array, 2, 1, 20, 1);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);

    GPIO_InitTypeDef ledgpio;
    ledgpio.GPIO_Pin = GPIO_Pin_9;
    ledgpio.GPIO_Mode = GPIO_Mode_OUT;
    ledgpio.GPIO_Speed = GPIO_Speed_Level_1;
    ledgpio.GPIO_OType = GPIO_OType_PP;
    ledgpio.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIO_Init(GPIOC, &ledgpio);

    BitAction value;
    for (value = 1;; value = !value) {
        GPIO_WriteBit(GPIOC, GPIO_Pin_9, value);
        uint32_t c;
        for (c = 0; c < 1000000; ++c);
    }

    return 0;
}
