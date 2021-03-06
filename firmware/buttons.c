#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_syscfg.h>
#include <stm32f0xx_exti.h>
#include <stm32f0xx_misc.h>
#include <stm32f0xx_pwr.h>

#include <deviceconfig.h>
#include <buttons.h>
#include <debugging.h>
#include <sysinit.h>

static unsigned BUTTON_MASK = 0;

unsigned buttons_get_mask()
{
    return BUTTON_MASK;
}
void buttons_clear_mask()
{
    BUTTON_MASK = 0;
}

static int32_t last_press_ticks = -1;
static uint32_t ticks_pressed_for = 0;

uint32_t buttons_get_ticks_pressed_for()
{
    if (last_press_ticks == -1)
        return 0;
    uint32_t extras = ((uint32_t)last_press_ticks) - SysTick->VAL;
    return extras + ticks_pressed_for;
}

// TODO: Might need to move this out of buttons.c if other logic needs to go
// in here.
void SysTick_Handler(void)
{
    if (last_press_ticks != -1) {
        ticks_pressed_for += last_press_ticks;
        //debugging_writec("P: ");
        //debugging_write_uint32(ticks_pressed_for);
        //debugging_writec("\n");
        last_press_ticks = SYS_TICK_MAX;
    }
}

static uint32_t last_tick = 0;
#define THRESHOLD 48000
void EXTI4_15_IRQHandler(void)
{
    // Clear interrupt flags. If we don't do this, this function will keep
    // getting called over and over due to a single button press.
    EXTI_ClearITPendingBit(PUSHBUTTON1_EXTI_LINE | PUSHBUTTON2_EXTI_LINE);

    uint32_t t = SysTick->VAL;

    // Figure out which pins are low.
    unsigned m = 0;
    m |= !GPIO_ReadInputDataBit(PUSHBUTTON1_GPIO_PORT, PUSHBUTTON1_PIN) << 1;
    m |= !GPIO_ReadInputDataBit(PUSHBUTTON2_GPIO_PORT, PUSHBUTTON2_PIN) << 2;

    if (m != 0) {
        if ((t < last_tick && last_tick - t > THRESHOLD) ||
            (t > last_tick && (last_tick + (16777215-t)) > THRESHOLD)) {
            BUTTON_MASK = m;
            last_press_ticks = (int)SysTick->VAL;

            debugging_writec("Button interrupt ");
            debugging_write_uint32(m);
            debugging_writec("\n");
        }
    }
    else {
        last_press_ticks = -1;
        ticks_pressed_for = 0;
    }

    last_tick = SysTick->VAL;
}

void buttons_setup()
{
    GPIO_InitTypeDef gpi;
    GPIO_StructInit(&gpi);
    gpi.GPIO_Mode = GPIO_Mode_IN;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_Pin = PUSHBUTTON1_PIN;
    gpi.GPIO_PuPd = GPIO_PuPd_UP;
    gpi.GPIO_Speed = GPIO_Speed_Level_1;
    GPIO_Init(PUSHBUTTON1_GPIO_PORT, &gpi);
    gpi.GPIO_Pin = PUSHBUTTON2_PIN;
    GPIO_Init(PUSHBUTTON2_GPIO_PORT, &gpi);

    SYSCFG_EXTILineConfig(PUSHBUTTON1_GPIO_PORT_SOURCE, PUSHBUTTON1_PIN_SOURCE);
    SYSCFG_EXTILineConfig(PUSHBUTTON2_GPIO_PORT_SOURCE, PUSHBUTTON2_PIN_SOURCE);

    EXTI_InitTypeDef exti;
    exti.EXTI_Line = PUSHBUTTON1_EXTI_LINE;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);
    exti.EXTI_Line = PUSHBUTTON2_EXTI_LINE;
    EXTI_Init(&exti);

    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = EXTI4_15_IRQn;
    nvic.NVIC_IRQChannelPriority = 0x03; // lowest
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}
