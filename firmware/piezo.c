#include <stdint.h>
#include <myassert.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_misc.h>
#include <deviceconfig.h>

void piezo_init()
{
    //
    // Test code that uses GPIO to bitbang the piezo pins.
    //

    /*GPIO_InitTypeDef gpi;
    gpi.GPIO_Pin = PIEZOPOS_PIN;
    gpi.GPIO_Mode = GPIO_Mode_OUT;
    gpi.GPIO_Speed = GPIO_Speed_Level_1;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(PIEZOPOS_GPIO_PORT, &gpi);
    gpi.GPIO_Pin = PIEZONEG_PIN;
    GPIO_Init(PIEZONEG_GPIO_PORT, &gpi);

    unsigned v = 0;
    for (;;) {
        GPIO_WriteBit(PIEZOPOS_GPIO_PORT, PIEZOPOS_PIN, v);
        GPIO_WriteBit(PIEZONEG_GPIO_PORT, PIEZONEG_PIN, !v);
        v = !v;
        unsigned i;
        for (i = 0; i < 1800; ++i);
    }*/


    //
    // PWM setup.
    //

    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    // GPIOA Configuration: TIM3 CH1 (PB4) and TIM3 CH2 (PB5).
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Connect TIM Channels to AF2.
        GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_3);

    // Compute the value to be set in ARR regiter to generate signal frequency at 17.57 Khz.
    uint16_t TimerPeriod = (SystemCoreClock / 910/*17570*/ ) - 1;
    // Compute CCR1 value to generate a duty cycle at 50% for channel 1 and 1N.
    uint16_t Channel1Pulse = (uint16_t) (((uint32_t) 5 * (TimerPeriod - 1)) / 10);
    // Compute CCR4 value to generate a duty cycle at 12.5%  for channel 4 */
    uint16_t Channel2Pulse = (uint16_t) (((uint32_t) 125 * (TimerPeriod- 1)) / 1000);

    // TIM3 clock enable.
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // Time base configuration.
    TIM_TimeBaseStructure.TIM_Period = 0;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // Channel 1 and 2 configuration in PWM mode.
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
    TIM_OCInitStructure.TIM_Pulse = Channel1Pulse;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_Pulse = Channel2Pulse;
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);

    // TIM3 counter enable.
    TIM_Cmd(TIM3, ENABLE);

    // Not needed for this timer (TIM3).
    //TIM_CtrlPWMOutputs(TIM3, ENABLE);
}
