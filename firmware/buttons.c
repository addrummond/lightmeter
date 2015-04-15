#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_syscfg.h>
#include <stm32f0xx_exti.h>
#include <stm32f0xx_misc.h>

#include <deviceconfig.h>
#include <buttons.h>
#include <debugging.h>

void EXTI4_15_IRQHandler(void)
{
    debugging_writec("Button interrupt x");
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
    exti.EXTI_Trigger = EXTI_Trigger_Falling;
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
