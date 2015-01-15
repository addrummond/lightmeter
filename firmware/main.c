#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_rcc.h>

int main()
{
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
