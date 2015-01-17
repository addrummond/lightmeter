#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_rcc.h>

#include <i2c.h>
#include <display.h>
#include <debugging.h>

int main()
{
    // Display reset pin uses GPIOA.
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);


    debugging_writec("Hello World!\n");

    i2c_init();
    debugging_writec("Here1\n");
    display_init();
    debugging_writec("Here2\n");

    debugging_writec("Here3\n");
    display_clear();
    debugging_writec("Here33\n");
    const uint8_t page_array[] = { 0xFF, 0xFF };
    display_write_page_array(page_array, 2, 1, 20, 1);
    debugging_writec("Here4\n");

    for (;;);
}
