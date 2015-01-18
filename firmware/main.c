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
    display_init();

    // Temporary test code: draw's rectangle on screen.
    display_clear();
    const uint8_t page_array[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    unsigned x;
    for (x = 20; x < 40; ++x)
        display_write_page_array(page_array, 2, 2, x, 1);

    for (;;);
}
