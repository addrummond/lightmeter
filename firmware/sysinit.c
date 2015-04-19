#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_pwr.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_misc.h>
#include <stm32f0xx_tim.h>
#include <sysinit.h>
#include <debugging.h>

#include <i2c.h>
#include <buttons.h>

void TIM3_IRQHandler()
{
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    debugging_writec("TIM!\n");
    //sysinit_enter_sleep_mode();
}

static void initialize_sleep_timer()
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = TIM3_IRQn;
    nvic.NVIC_IRQChannelPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    const uint16_t PRE = 6000;
    TIM_TimeBaseInitTypeDef tbi;
    tbi.TIM_Period = 10000;
    tbi.TIM_Prescaler = 0;
    tbi.TIM_ClockDivision = 0;
    tbi.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &tbi);

    TIM_PrescalerConfig(TIM3, PRE, TIM_PSCReloadMode_Immediate);

    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

void sysinit_init()
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);

    SysTick->LOAD = 16777215;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    i2c_init();
    buttons_setup();

    initialize_sleep_timer();
}

void sysinit_enter_sleep_mode()
{
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_SLEEPEntry_WFI);
}
