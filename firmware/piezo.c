#include <myassert.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include <goetzel.h>
#include <piezo.h>
#include <hfsdp.h>
#include <deviceconfig.h>
#include <debugging.h>
#include <systime.h>


//
// Microphone stuff.
//

//#define MIC_OFFSET_ADC_V ((int32_t)((1.3/3.3)*4096.0))
// Empirically determined.
#define MIC_OFFSET_ADC_V 1720

__IO int16_t piezo_mic_buffer[PIEZO_MIC_BUFFER_N_SAMPLES];

static inline void dma_config()
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
    dmai.DMA_Mode = DMA_Mode_Normal;
    dmai.DMA_Priority = DMA_Priority_High;
    dmai.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &dmai);
}

void piezo_mic_init()
{
    //
    // Turn on SCRPWR, which also powers mic.
    //
    GPIO_InitTypeDef gpi;
    GPIO_StructInit(&gpi);
    gpi.GPIO_Pin = DISPLAY_POWER_PIN;
    gpi.GPIO_Mode = GPIO_Mode_OUT;
    gpi.GPIO_Speed = GPIO_Speed_Level_1;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(DISPLAY_POWER_GPIO_PORT, &gpi);
    // It's a P-channel MOSFET so we set the pin low to turn it on.
    GPIO_WriteBit(DISPLAY_POWER_GPIO_PORT, DISPLAY_POWER_PIN, 0);

    //
    // Configure timer that will be used to trigger ADC at 44.1KHz.
    // Timer appears to run on 8HMz clock (this is not abundantly clear from
    // the clock tree in the STM32F0 reference docs).
    //
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    TIM_DeInit(TIM1);
    TIM_TimeBaseInitTypeDef tbi;
    TIM_TimeBaseStructInit(&tbi);
    tbi.TIM_Prescaler = 0;
    tbi.TIM_Period =    //181-1; // 44.1KHz
                        91-1;  // 88.2KHz
    tbi.TIM_ClockDivision = TIM_CKD_DIV1;
    tbi.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &tbi);

    //TIM_SelectOutputTrigger(TIM1, TIM_C);

    TIM_OCInitTypeDef toci;
    TIM_OCStructInit(&toci);
    toci.TIM_OCMode = TIM_OCMode_PWM1;
    toci.TIM_OutputState = TIM_OutputState_Enable;
    toci.TIM_Pulse = 0x01;
    TIM_OC4Init(TIM1, &toci);

    TIM_Cmd(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    //
    // ADC configuration.
    //
    ADC_InitTypeDef adci;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    GPIO_StructInit(&gpi);
    gpi.GPIO_Pin = GPIO_Pin_1;
    gpi.GPIO_Mode = GPIO_Mode_AN;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &gpi);

    ADC_DeInit(ADC1);

    ADC_StructInit(&adci);
    adci.ADC_Resolution = ADC_Resolution_12b;
    adci.ADC_ContinuousConvMode = DISABLE;
    //adci.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    adci.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
    adci.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC4;
    adci.ADC_DataAlign = ADC_DataAlign_Right;
    adci.ADC_ScanDirection = ADC_ScanDirection_Upward;
    ADC_Init(ADC1, &adci);

    ADC_ClockModeConfig(ADC1, ADC_ClockMode_AsynClk);

    ADC_ChannelConfig(ADC1, ADC_Channel_9, ADC_SampleTime_13_5Cycles);
    ADC_GetCalibrationFactor(ADC1);

    ADC_DMARequestModeConfig(ADC1, ADC_DMAMode_OneShot);
    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));
}

void piezo_mic_deinit()
{

}

void piezo_mic_read_buffer()
{
#define sbuf ((int16_t *)piezo_mic_buffer)
#define ubuf ((uint16_t *)piezo_mic_buffer)

    //uint32_t jt = SysTick->VAL;

    while (! (ADC1->ISR & ADC_FLAG_ADRDY));
    ADC1->CFGR1 |= ADC_CFGR1_WAIT;
    ADC1->CR |= ADC_FLAG_ADSTART;
    dma_config();
    DMA1_Channel1->CCR |= DMA_CCR_EN;
    while ((DMA1->ISR & DMA1_FLAG_TC1) == RESET);

    /*while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));
    ADC_WaitModeCmd(ADC1, ENABLE);
    ADC_StartOfConversion(ADC1);
    dma_config();
    DMA_Cmd(DMA1_Channel1, ENABLE);
    while (DMA_GetFlagStatus(DMA1_FLAG_TC1) == RESET);*/

    //debugging_writec("JITTER: ");
    //debugging_write_uint32(jt-jt2);
    //debugging_writec("\n");

    DMA_ClearFlag(DMA1_FLAG_TC1);
    DMA_Cmd(DMA1_Channel1, DISABLE);

    unsigned i;
    int32_t mean = 0;
    for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES; ++i)
        mean += ubuf[i];
    mean /= PIEZO_MIC_BUFFER_N_SAMPLES;

    // Convert each value to signed 16-bit value using appropriate offset.
    for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES; ++i) {
        sbuf[i] -= mean;
    }

#undef sbuf
#undef ubuf
}

int32_t piezo_get_magnitude()
{
    int32_t total = 0;
    unsigned i;
    for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES; ++i) {
        int32_t v = piezo_mic_buffer[i];
        total += v*v;
    }
    return (total/PIEZO_MIC_BUFFER_N_SAMPLES);
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

//static int32_t debugbuf[PIEZO_MIC_BUFFER_N_SAMPLES*2];
//static int32_t debugbufi;
bool piezo_read_data(uint8_t *buffer, unsigned bytes)
{
    unsigned bits = bytes*8;

    hfsdp_read_bit_state_t s;
    init_hfsdp_read_bit_state(&s, HFSDP_MASTER_CLOCK_COSCOEFF, HFSDP_MASTER_CLOCK_SINCOEFF,
                                  HFSDP_MASTER_DATA_COSCOEFF,  HFSDP_MASTER_DATA_SINCOEFF);

    //debugbufi = 0;

    bool started = false;
    unsigned nreceived = 0;
    SYSTIME_START(t);
    for (;;) {
        piezo_mic_read_buffer();

        if (! started) {
            started = hfsdp_check_start(&s, (const int16_t *)piezo_mic_buffer, PIEZO_MIC_BUFFER_N_SAMPLES);
        }
        else {
            int r = hfsdp_read_bit(&s, (const int16_t *)piezo_mic_buffer, PIEZO_MIC_BUFFER_N_SAMPLES);

            if (r == HFSDP_READ_BIT_DECODE_ERROR)
                return false;
            else if (r == HFSDP_READ_BIT_NOTHING_READ)
                ;//debugging_writec("N\n");
            else {
                unsigned shiftup = nreceived % 8;
                if (shiftup == 0)
                    buffer[nreceived] = r;
                else
                    buffer[nreceived] |= (r << shiftup);
                ++nreceived;

                if (nreceived == bits)
                    return true;
            }
        }

        // Wait until it's time to take the next sample.
        SYSTIME_WAIT_UNTIL_CYCLES(t, HFSDP_SAMPLE_CYCLES);

        //uint32_t v = SysTick->VAL;
        //debugging_writec("TE: ");
        //debugging_write_uint32(t.st - v);
        //debugging_writec("\n");

        SYSTIME_UPDATE(t);
    }
}
