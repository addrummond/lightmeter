#include <myassert.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_adc.h>
#include <stm32f0xx_misc.h>
#include <stm32f0xx_dma.h>
#include <goetzel.h>
#include <piezo.h>
#include <deviceconfig.h>
#include <debugging.h>


//
// Microphone stuff.
//

__IO int16_t piezo_mic_buffer[PIEZO_MIC_BUFFER_N_SAMPLES];

#define USE_DMA
//#undef USE_DMA

#ifdef USE_DMA
static void dma_config()
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_InitTypeDef dmai;

    // DMA1 Channel1 Config.
    DMA_DeInit(DMA1_Channel1);
    dmai.DMA_PeripheralBaseAddr = (uint32_t)(&(ADC1->DR));
    dmai.DMA_MemoryBaseAddr = (uint32_t)piezo_mic_buffer;
    dmai.DMA_DIR = DMA_DIR_PeripheralSRC;
    dmai.DMA_BufferSize = PIEZO_MIC_BUFFER_N_SAMPLES;
    dmai.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dmai.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dmai.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dmai.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dmai.DMA_Mode = DMA_Mode_Circular;
    dmai.DMA_Priority = DMA_Priority_High;
    dmai.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &dmai);
    // DMA1 Channel1 enable.
    DMA_Cmd(DMA1_Channel1, ENABLE);
}
#endif

void piezo_mic_init()
{
    //
    // Turn on SCRPWR, which also powers mic.
    //
    GPIO_WriteBit(DISPLAY_POWER_GPIO_PORT, DISPLAY_POWER_PIN, 0);

    //
    // ADC configuration.
    //
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

#ifdef USE_DMA
    ADC_DMARequestModeConfig(ADC1, ADC_DMAMode_Circular);
    ADC_DMACmd(ADC1, ENABLE);
#endif

    ADC_Cmd(ADC1, ENABLE);

    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));
    ADC_StartOfConversion(ADC1);

#ifdef USE_DMA
    // DMA1 clock enable.
    dma_config();
#endif
}

void piezo_mic_deinit()
{

}

#ifndef USE_DMA
static void piezo_mic_wait_on_ready()
{
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
}
#endif

static void piezo_mic_read_buffer()
{
#ifdef USE_DMA
    while((DMA_GetFlagStatus(DMA1_FLAG_TC1)) == RESET);
    DMA_ClearFlag(DMA1_FLAG_TC1);
#else
    unsigned i;
    for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES; ++i, piezo_mic_wait_on_ready()) {
        piezo_mic_buffer[i] = ADC_GetConversionValue(ADC1);
    }
#endif
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
    for (;;) {
        //uint32_t before = SysTick->VAL;

        piezo_mic_read_buffer();
        DMA_Cmd(DMA1_Channel1, DISABLE);

        /*unsigned i;
        for (i = 0; i < 6; ++i) {
            debugging_write_uint32((int32_t)(piezo_mic_buffer[i] - 4096/2));
            debugging_writec("\n");
        }
        debugging_writec("\n---\n");*/

        int p = goetzel((const uint16_t *)piezo_mic_buffer, PIEZO_MIC_BUFFER_N_SAMPLES, PIEZO_HFSDP_A_MODE_MASTER_CLOCK_COEFF);

        /*unsigned i;
        for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES; ++i)
            piezo_mic_buffer[i] = 3;
        for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES; ++i) {
            debugging_writec("V: ");
            debugging_write_uint32(piezo_mic_buffer[i]);
            debugging_writec("\n");
        }*/

        DMA_Cmd(DMA1_Channel1, ENABLE);

        //debugging_writec("val ");
        //debugging_write_uint32(p);
        //debugging_writec("\n");

        //uint32_t after = SysTick->VAL;
        //debugging_writec("time ");
        //debugging_write_uint32(before-after);
        //debugging_writec("\n");

        if (p > 30000)
            return true;
    }
}
