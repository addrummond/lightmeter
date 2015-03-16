#include <myassert.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_adc.h>
#include <stm32f0xx_misc.h>
#include <fix_fft.h>
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

#define N_SAMPLES (PIEZO_MIC_BUFFER_SIZE_BYTES/2)

void piezo_mic_read_buffer(int8_t *samples)
{
    int16_t *samples16 = (int16_t *)samples;
    unsigned i;
    for (i = 0; i < N_SAMPLES; ++i, piezo_mic_wait_on_ready()) {
        samples16[i] = (int16_t)(piezo_mic_get_reading()) - (4096/2);
    }
    samples16[0] = samples16[1];

    // 12 bits to 8.
    int16_t raw_max = -(4096/2);
    for (i = 0; i < N_SAMPLES; ++i) {
        int16_t v = samples16[i];
        if (v < 0)
            v = -v;
        if (v > raw_max)
            raw_max = v;
    }
    int16_t ratio = 127/raw_max;
    if (ratio > 1) {
        for (i = 0; i < N_SAMPLES; ++i)
            samples16[i] *= ratio;
    }
    raw_max = -(4096/2);
    for (i = 0; i < N_SAMPLES; ++i) {
        int16_t v = samples16[i];
        if (v < 0)
            v = -v;
        if (v > raw_max)
            raw_max = v;
    }
    unsigned shift = 0;
    if (raw_max >= 1024)
        shift = 4;
    else if (raw_max >= 512)
        shift = 3;
    else if (raw_max >= 256)
        shift = 2;
    else if (raw_max >= 128)
        shift = 1;
    for (i = 0; i < N_SAMPLES; ++i)
        samples[i] = samples16[i] >> shift;
}

uint32_t piezo_mic_buffer_get_sqmag(int8_t *samples)
{
    uint32_t sq = 0;
    unsigned i;
    for (i = 0; i < N_SAMPLES; ++i)
        sq += samples[i] * samples[i];
    return sq;
}

void piezo_mic_buffer_fft(int8_t *samples)
{
    // Find first and second formant.
    int8_t *imaginary = samples + N_SAMPLES;
    unsigned i;
    for (i = 0; i < N_SAMPLES; ++i)
        imaginary[i] = 0;

    fix_fft(samples, imaginary, PIEZO_SHIFT_1_BY_THIS_TO_GET_N_SAMPLES, 0);
}

void piezo_fft_buffer_get_12formants(int8_t *samples,
                                     unsigned *first_formant,
                                     unsigned *second_formant,
                                     unsigned *first_formant_mag,
                                     unsigned *second_formant_mag)
{
    int8_t *imaginary = samples + N_SAMPLES;

    int32_t max = 0, max2 = 0;
    unsigned maxi = 0, max2i = 0;
    // Starts from 1 because sample is DC component (I think). If not, it's too
    // low a frequency to be of interest anyway.
    unsigned i;
    for (i = 1; i < 60/* cut off frequencies above ~10kHz*/; ++i) {
        int32_t e = samples[i]*samples[i] + imaginary[i]*imaginary[i];

        if (max < e) {
            max = e;
            maxi = i;
        }
        if (max2 < e && e < max) {
            max2 = e;
            max2i = i;
        }
    }

    if (first_formant)
        *first_formant = maxi;
    if (second_formant)
        *second_formant = max2i;
    if (first_formant_mag)
        *first_formant_mag = max;
    if (second_formant_mag)
        *second_formant_mag = max2;
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

#define SQMAG_THRESHOLD 10

bool piezo_hfsdp_listen_for_masters_init()
{
    int8_t samples[PIEZO_MIC_BUFFER_SIZE_BYTES];
    int8_t *imaginary = samples + N_SAMPLES;

    unsigned last_state = 0; // 1 = F1 on F2 off, 2 = F2 on F1 off.
    unsigned state_changes = 0;
    unsigned count = 0;
    for (state_changes = 0; state_changes < 16; ++count) {
        if (count > 200)
            state_changes = 0;

        piezo_mic_read_buffer(samples);
        piezo_mic_buffer_fft(samples);

        const unsigned SLACK = 0;
        const unsigned NBANDS = SLACK*2 + 1;
        uint32_t ranges[NBANDS*2];
        uint32_t *m1range = ranges;
        uint32_t *m2range = ranges+NBANDS;

        int i;
        for (i = 0; i < NBANDS; ++i)
            ranges[i] = 0;

        unsigned j;
        for (i = -SLACK, j = 0; i < SLACK+1; ++i, ++j) {
            int m1i = PIEZO_HFSDP_A_MODE_MASTER_F1_FTF + i;
            int m2i = PIEZO_HFSDP_A_MODE_MASTER_F2_FTF + i;

            if (m1i > 0)
                m1range[j] = (int32_t)samples[m1i]*(int32_t)samples[m1i] + (int32_t)imaginary[m1i]*(int32_t)imaginary[m1i];
            if (m2i > 0)
                m2range[j] = (int32_t)samples[m2i]*(int32_t)samples[m2i] + (int32_t)imaginary[m2i]*(int32_t)imaginary[m2i];
        }

        uint32_t m1max = 0, m2max = 0;
        for (i = 0; i < NBANDS; ++i) {
            if (m1range[i] > m1max)
                m1max = m1range[i];
            if (m2range[i] > m2max)
                m2max = m2range[i];
        }

        unsigned new_state = 0;
        if (m1max > m2max && m1max - m2max >= SQMAG_THRESHOLD)
            new_state = 1;
        else if (m2max > m1max && m2max - m1max >= SQMAG_THRESHOLD)
            new_state = 2;

        if (last_state == 0) {
            if (new_state > 0) {
                last_state = new_state;
                ++state_changes;
            }
        }
        else if (new_state > 0 && last_state != new_state) {
            ++state_changes;
            last_state = new_state;
        }
    }

    return true;
}
