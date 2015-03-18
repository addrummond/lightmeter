#include <myassert.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_adc.h>
#include <stm32f0xx_misc.h>
#include <goetzel.h>
#include <piezo.h>
#include <deviceconfig.h>
#include <debugging.h>

//
// Microphone stuff.
//

void piezo_mic_init()
{
    ADC_InitTypeDef adci;
    GPIO_InitTypeDef gpi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    gpi.GPIO_Pin = GPIO_Pin_4;
    gpi.GPIO_Mode = GPIO_Mode_AN;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpi);

    ADC_DeInit(ADC1);

    ADC_StructInit(&adci);
    adci.ADC_Resolution = ADC_Resolution_12b;
    adci.ADC_ContinuousConvMode = ENABLE;
    adci.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    adci.ADC_DataAlign = ADC_DataAlign_Right;
    adci.ADC_ScanDirection = ADC_ScanDirection_Upward;
    ADC_Init(ADC1, &adci);

    ADC_ChannelConfig(ADC1, ADC_Channel_4, ADC_SampleTime_239_5Cycles);
    ADC_GetCalibrationFactor(ADC1);
    //ADC_ContinuousModeCmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));
    ADC_StartOfConversion(ADC1);
}

static void piezo_mic_wait_on_ready()
{
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
}

static uint16_t piezo_mic_get_reading()
{
    return ADC_GetConversionValue(ADC1);
}

void piezo_mic_read_buffer(int16_t *samples)
{
    unsigned i;
    for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES; ++i, piezo_mic_wait_on_ready()) {
        samples[i] = (int16_t)(piezo_mic_get_reading()) - (4096/2);
    }
}

int32_t piezo_mic_buffer_get_sqmag(int16_t *samples)
{
    int32_t sq = 0;
    unsigned i;
    for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES; ++i)
        sq += samples[i] * samples[i];
    return sq;
}


//
// Piezo buzzer stuff.
//

void piezo_out_init()
{
    // GPIOA configuration: TIM3 CH1 (PA6) and TIM3 CH2 (PA7),
    //                      TIM1 CH3 (PA10) and TIM1 CH4 (PA11).
    GPIO_InitTypeDef gpi;
    gpi.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_10 | GPIO_Pin_11;
    gpi.GPIO_Mode = GPIO_Mode_AF;
    gpi.GPIO_Speed = GPIO_Speed_50MHz;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_UP ;
    GPIO_Init(GPIOA, &gpi);

    // Connect TIM3 channels to AF1.
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_1);

    // Connect TIM1 channels to AF2.
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_2);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
}

void piezo_set_period(unsigned channels, uint16_t period)
{
    // 50% duty cycle.
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
    if (channels & 1)
        TIM_OC2Init(TIM3, &oci);
    if (channels & 2)
        TIM_OC3Init(TIM1, &oci);
    oci.TIM_OCMode = TIM_OCMode_PWM2;
    if (channels & 1)
        TIM_OC1Init(TIM3, &oci);
    if (channels & 2)
        TIM_OC4Init(TIM1, &oci);

    // Time base configuration.
    tbs.TIM_Prescaler = 0;
    tbs.TIM_CounterMode = TIM_CounterMode_Up;
    tbs.TIM_Period = period;
    tbs.TIM_ClockDivision = 0;
    tbs.TIM_RepetitionCounter = 0;
    if (channels & 1)
        TIM_TimeBaseInit(TIM3, &tbs);
    if (channels & 2)
        TIM_TimeBaseInit(TIM1, &tbs);
}

void piezo_turn_on(unsigned channels)
{
    // TIM3 clock enable.
    if (channels & 1) {
        TIM_Cmd(TIM3, ENABLE);
    }
    if (channels & 2) {
        TIM_Cmd(TIM1, ENABLE);
        TIM_CtrlPWMOutputs(TIM1, ENABLE);
    }
}

static uint32_t old_channel1_period = 0;
static uint16_t old_channel2_period = 0;
void piezo_pause(unsigned channels)
{
    if (channels & 1) {
        old_channel1_period = TIM3->ARR;
        TIM3->ARR = 0;
    }
    if (channels & 2) {
        old_channel2_period = TIM1->ARR;
        TIM1->ARR = 0;
    }
}

void piezo_unpause(unsigned channels)
{
    if (channels & 1) {
        TIM3->ARR = old_channel1_period;
        //TIM3->EGR = TIM_PSCReloadMode_Immediate;
    }
    if (channels & 2) {
        TIM1->ARR = old_channel2_period;
        //TIM1->EGR = TIM_PSCReloadMode_Immediate;
    }
}

void piezo_out_deinit()
{
    TIM_Cmd(TIM3, DISABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, DISABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, DISABLE);
}


//
// HFSDP stuff.
//

bool piezo_hfsdp_listen_for_masters_init()
{
    int16_t samples[PIEZO_MIC_BUFFER_N_SAMPLES];
    for (;;) {
        piezo_mic_read_buffer(samples);
        goetzel_state_t gs;
        goetzel_state_init(&gs, PIEZO_HFSDP_A_MODE_MASTER_CLOCK_COEFF);
        unsigned i;
        for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES; ++i) {
            goetzel_step(&gs, samples[i]);
        }
        int32_t p = goetzel_get_normalized_power(&gs);
        if (p > 20000)
            return true;
    }
}
