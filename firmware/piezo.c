#include <stdint.h>
#include <myassert.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_misc.h>
#include <deviceconfig.h>

void piezo_init()
{
    // GPIOB configuration: TIM3 CH1 (PB4) and TIM3 CH2 (PB5).
    GPIO_InitTypeDef gpi;
    gpi.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
    gpi.GPIO_Mode = GPIO_Mode_AF;
    gpi.GPIO_Speed = GPIO_Speed_50MHz;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_UP ;
    GPIO_Init(GPIOB, &gpi);

    // GPIOA configuration: TIM1 CH3 (PA10) and TIM1 CH4 (PA11).
    gpi.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_Init(GPIOA, &gpi);

    // Connect TIM3 channels to AF1.
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_1);

    // Connect TIM1 channels to AF2.
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_2);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
}

void piezo_set_periods(uint16_t period1, uint16_t period2)
{
    // 50% duty cycle.
    uint16_t pulse1 = (uint16_t) (((uint32_t) 5 * (period1 - 1)) / 10);
    uint16_t pulse2 = (uint16_t) (((uint32_t) 5 * (period2 - 1)) / 10);

    TIM_TimeBaseInitTypeDef tbs;
    TIM_OCInitTypeDef oci;

    // TIM3 channel 1 and 2 configuration in PWM mode.
    oci.TIM_OCMode = TIM_OCMode_PWM1;
    oci.TIM_OutputState = TIM_OutputState_Enable;
    oci.TIM_OutputNState = TIM_OutputNState_Enable;
    oci.TIM_Pulse = pulse1;
    oci.TIM_OCPolarity = TIM_OCPolarity_Low;
    oci.TIM_OCNPolarity = TIM_OCNPolarity_High;
    oci.TIM_OCIdleState = TIM_OCIdleState_Set;
    oci.TIM_OCNIdleState = TIM_OCIdleState_Reset;
    TIM_OC1Init(TIM3, &oci);
    oci.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OC2Init(TIM3, &oci);

    // TIM1 channel 3 and 4 configuration in PWM mode.
    oci.TIM_Pulse = pulse2;
    oci.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OC3Init(TIM1, &oci);
    oci.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OC4Init(TIM2, &oci);

    // Time base configuration.
    tbs.TIM_Prescaler = 0;
    tbs.TIM_CounterMode = TIM_CounterMode_Up;
    tbs.TIM_Period = period1;
    tbs.TIM_ClockDivision = 0;
    tbs.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &tbs);
    tbs.TIM_Period = period2;
    TIM_TimeBaseInit(TIM1, &tbs);
}

void piezo_turn_on()
{
    // TIM3 clock enable.
    TIM_Cmd(TIM3, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
}

void piezo_pause()
{
    TIM_Cmd(TIM3, DISABLE);
    TIM_Cmd(TIM1, DISABLE);
    TIM_CtrlPWMOutputs(TIM1, DISABLE);
}

void piezo_deinit()
{
    TIM_Cmd(TIM3, DISABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, DISABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, DISABLE);
}
