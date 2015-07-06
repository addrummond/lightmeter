#include <myassert.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include <goetzel.h>
#include <piezo.h>
#include <deviceconfig.h>
#include <debugging.h>


//
// Microphone stuff.
//

//#define MIC_OFFSET_ADC_V ((int32_t)((1.3/3.3)*4096.0))
// Empirically determined.
#define MIC_OFFSET_ADC_V 1113

// Some extra space at beginning to absorb junk values from first conversions.
#define JUNK_SPACE 1
__IO int16_t piezo_mic_buffer_[PIEZO_MIC_BUFFER_N_SAMPLES+JUNK_SPACE];
__IO int16_t *piezo_mic_buffer = piezo_mic_buffer_ + JUNK_SPACE;

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
    dmai.DMA_BufferSize = PIEZO_MIC_BUFFER_N_SAMPLES+JUNK_SPACE;
    dmai.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dmai.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dmai.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dmai.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dmai.DMA_Mode = DMA_Mode_Normal;
    dmai.DMA_Priority = DMA_Priority_High;
    dmai.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &dmai);
    // DMA1 Channel1 enable.
    DMA_Cmd(DMA1_Channel1, ENABLE);
}

// TODO: See if we can trim this down a bit.
static void dma_restart()
{
    dma_config();
}

#endif

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
    adci.ADC_ContinuousConvMode = ENABLE;
    adci.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    adci.ADC_DataAlign = ADC_DataAlign_Right;
    adci.ADC_ScanDirection = ADC_ScanDirection_Upward;
    ADC_Init(ADC1, &adci);

    ADC_ClockModeConfig(ADC1, ADC_ClockMode_AsynClk);

    ADC_ChannelConfig(ADC1, ADC_Channel_9, ADC_SampleTime_239_5Cycles);
    ADC_GetCalibrationFactor(ADC1);

#ifdef USE_DMA
    ADC_DMARequestModeConfig(ADC1, ADC_DMAMode_OneShot);
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

void piezo_mic_read_buffer()
{
#define sbuf ((int16_t *)piezo_mic_buffer_)
#define ubuf ((uint16_t *)piezo_mic_buffer_)

    unsigned i;

#ifdef USE_DMA
    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));
    ADC_StartOfConversion(ADC1);
    dma_restart();
    while((DMA_GetFlagStatus(DMA1_FLAG_TC1)) == RESET);
    DMA_ClearFlag(DMA1_FLAG_TC1);
#else
    for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES+JUNK_SPACE; ++i, piezo_mic_wait_on_ready()) {
        uint16_t v = ADC_GetConversionValue(ADC1);
        piezo_mic_buffer_[i] = v;
    }
#endif

    // Convert each value to signed 16-bit value using appropriate offset.
    for (i = JUNK_SPACE; i < PIEZO_MIC_BUFFER_N_SAMPLES+JUNK_SPACE; ++i) {
        if (ubuf[i] < MIC_OFFSET_ADC_V)
            sbuf[i] = -(int16_t)(MIC_OFFSET_ADC_V - ubuf[i]);
        else
            sbuf[i] = (int16_t)(ubuf[i]-MIC_OFFSET_ADC_V);
    }

#undef SUBTRACT_EVERY
#undef TO_SUBTRACT

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

bool piezo_hfsdp_listen_for_masters_init()
{
    for (;;) {
        //uint32_t before = SysTick->VAL;

        piezo_mic_read_buffer();

        // unsigned i;
        // for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES; ++i) {
        //    debugging_write_int32(piezo_mic_buffer[i]);
        //    debugging_writec("\n");
        // }
        // debugging_writec("\n---\n");

        // debugging_writec("M: ");
        // debugging_write_uint32(piezo_get_magnitude());
        // debugging_writec("\n");

        //unsigned t = SysTick->VAL;

        int32_t p1, p2;
        goetzel2((const int16_t *)piezo_mic_buffer, PIEZO_MIC_BUFFER_N_SAMPLES,
                 PIEZO_HFSDP_A_MODE_MASTER_CLOCK_COEFF,
                 PIEZO_HFSDP_A_MODE_MASTER_DATA_COEFF,
                 &p1, &p2);

        // unsigned t2 = SysTick->VAL;
        // t -= t2;
        // debugging_writec("CYCLES: ");
        // debugging_write_uint32(t);
        // debugging_writec("\n");

        // unsigned i;
        // for (i = 0; i < PIEZO_MIC_BUFFER_N_SAMPLES; ++i) {
        //     debugging_writec("V: ");
        //     debugging_write_uint32(piezo_mic_buffer[i]);
        //     debugging_writec("\n");
        // }

        debugging_writec("vals ");
        debugging_write_int32(p1);
        debugging_writec(" ");
        debugging_write_int32(p2);
        debugging_writec("\n");

        //uint32_t after = SysTick->VAL;
        //debugging_writec("time ");
        //debugging_write_uint32(before-after);
        //debugging_writec("\n");

        if (p1 > 30000)
            return true;
    }
}
