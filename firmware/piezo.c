#include <stdint.h>
#include <myassert.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_misc.h>
#include <deviceconfig.h>

void piezo_init()
{
    // GPIOA Configuration: TIM3 CH1 (PB4) and TIM3 CH2 (PB5).
    GPIO_InitTypeDef gpi;
    gpi.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
    gpi.GPIO_Mode = GPIO_Mode_AF;
    gpi.GPIO_Speed = GPIO_Speed_50MHz;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_UP ;
    GPIO_Init(GPIOB, &gpi);

    // Connect TIM Channels to AF1.
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_1);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

}

void piezo_set_period(uint16_t period)
{
    uint16_t pulse = (uint16_t) (((uint32_t) 5 * (period - 1)) / 10);

    TIM_TimeBaseInitTypeDef tbs;
    TIM_OCInitTypeDef oci;

    // Channel 1 and 2 configuration in PWM mode.
    oci.TIM_OCMode = TIM_OCMode_PWM1;
    oci.TIM_OutputState = TIM_OutputState_Enable;
    oci.TIM_OutputNState = TIM_OutputNState_Enable;
    oci.TIM_Pulse = pulse;
    oci.TIM_OCPolarity = TIM_OCPolarity_Low;
    oci.TIM_OCNPolarity = TIM_OCNPolarity_High;
    oci.TIM_OCIdleState = TIM_OCIdleState_Set;
    oci.TIM_OCNIdleState = TIM_OCIdleState_Reset;
    TIM_OC1Init(TIM3, &oci);
    oci.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OC2Init(TIM3, &oci);

    // Time base configuration.
    tbs.TIM_Prescaler = 0;
    tbs.TIM_CounterMode = TIM_CounterMode_Up;
    tbs.TIM_Period = period;
    tbs.TIM_ClockDivision = 0;
    tbs.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &tbs);
}

void piezo_turn_on()
{
    // TIM3 clock enable.
    TIM_Cmd(TIM3, ENABLE);
}

void piezo_pause()
{
    TIM_Cmd(TIM3, DISABLE);
}

void piezo_deinit()
{
    TIM_Cmd(TIM3, DISABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, DISABLE);
}
