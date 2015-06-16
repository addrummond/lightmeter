#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_adc.h>
#include <stm32f0xx_misc.h>
#include <stm32f0xx_dma.h>

#include <tables.h>
#include <deviceconfig.h>
#include <meter.h>
#include <debugging.h>
#include <exposure.h>

#define CHAN (ADC_Channel_1 | ADC_Channel_2)

static meter_mode_t current_mode;

#define MODE_TO_DIODESW(m) ((m) == METER_MODE_REFLECTIVE)

void meter_set_mode(meter_mode_t mode)
{
    GPIO_WriteBit(DIODESW_GPIO_PORT, DIODESW_PIN, MODE_TO_DIODESW(mode));
    current_mode = mode;
}

#define fast_set_channel(channel)  (ADC1->CHSELR = (channel))
#define fast_set_sample_time(time) ((ADC1->SMPR &= ADC_SMPR1_SMPR), (ADC1->SMPR |= (time)))

static uint16_t adc_buffer[2];

void meter_init()
{
    GPIO_InitTypeDef gpi;

    //
    // Init DIODESW. INTEGCLR is already set up in sysinit.
    //
    gpi.GPIO_Pin = DIODESW_PIN;
    gpi.GPIO_Mode = GPIO_Mode_OUT;
    gpi.GPIO_Speed = GPIO_Speed_Level_3;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(DIODESW_GPIO_PORT, &gpi);

    //
    // Init ADC.
    //
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    gpi.GPIO_Pin = GPIO_Pin_1;
    gpi.GPIO_Mode = GPIO_Mode_AN;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpi);
    gpi.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOA, &gpi);

    ADC_DeInit(ADC1);

    ADC_InitTypeDef adci;
    ADC_StructInit(&adci);
    adci.ADC_Resolution = ADC_Resolution_12b;
    adci.ADC_ContinuousConvMode = DISABLE;
    adci.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    adci.ADC_DataAlign = ADC_DataAlign_Right;
    adci.ADC_ScanDirection = ADC_ScanDirection_Upward;
    ADC_Init(ADC1, &adci);

    ADC1->CHSELR = CHAN;
    ADC_ChannelConfig(ADC1, CHAN, ADC_SampleTime_13_5Cycles);
    ADC_GetCalibrationFactor(ADC1);

    ADC_DMARequestModeConfig(ADC1, ADC_DMAMode_OneShot);
    ADC_DMACmd(ADC1, ENABLE);

    ADC_Cmd(ADC1, ENABLE);
    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_InitTypeDef dmai;

    // DMA1 Channel1 Config.
    DMA_DeInit(DMA1_Channel1);
    dmai.DMA_PeripheralBaseAddr = (uint32_t)(&(ADC1->DR));
    dmai.DMA_MemoryBaseAddr = (uint32_t)adc_buffer;
    dmai.DMA_DIR = DMA_DIR_PeripheralSRC;
    dmai.DMA_BufferSize = sizeof(adc_buffer)/sizeof(uint16_t);
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

uint32_t meter_take_raw_nonintegrated_reading()
{
    fast_set_channel(CHAN);
    fast_set_sample_time(ADC_SampleTime_239_5Cycles);

    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 1);
    // Wait a bit for things to stabilize.
    unsigned i;
    for (i = 0; i < 3000; ++i);

    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));

    // Following line is equivalent to:
    //     ADC_StartOfConversion(ADC1); // Function call overhead is significant.
    ADC1->CR |= (uint32_t)ADC_CR_ADSTART;

    // Following line is equivalent to:
    //     while((DMA_GetFlagStatus(DMA1_FLAG_TC1)) == RESET);
    while ((DMA1->ISR & DMA1_FLAG_TC1) == RESET);

    return adc_buffer[0] | (adc_buffer[1] << 16);
}

#define st(x) STAGE ## x ##_TICKS,
static uint32_t STAGES[] = {
    FOR_EACH_AMP_STAGE(st)
};
#undef st

void meter_take_raw_integrated_readings(uint16_t *outputs)
{
    fast_set_channel(CHAN);
    fast_set_sample_time(ADC_SampleTime_13_5Cycles);

    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));

    // Close switch to discharge integrating cap.
    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 1);
    unsigned i;
    for (i = 0; i < 3000; ++i); // Leave some time for cap to discharge.

    // Open the switch.
    //
    // Following line is equivalent to:
    //     GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 0);
    INTEGCLR_GPIO_PORT->BRR = INTEGCLR_PIN;

    // From here to first check against SysTick takes 21 cycles.
    // ADC conversion itself takes 20 cycles.
    uint32_t st = SysTick->VAL;

    //uint32_t st2 = SysTick->VAL;
    //debugging_writec("GAP: ");
    //debugging_write_uint32(st-st2);
    //debugging_writec("\n");

    // Determine value of SysTick for each endpoint.
    uint32_t current_endpoint = st - STAGES[i];
    bool lt = (current_endpoint <= st);

    // Appears not to be necessary. (Has no discernable effect on accuracy.)
    //
    // while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));

    //uint32_t st2 = SysTick->VAL;
    //debugging_writec("GAP: ");
    //debugging_write_uint32(st-st2);
    //debugging_writec("\n");

    // Read cap voltage at each stage.
    unsigned oi = 0;
    for (i = 0;;) {
        //uint32_t stb = SysTick->VAL;
        //debugging_writec("GAP: ");
        //debugging_write_uint32(st-stb);
        //debugging_writec("\n");

        if ((lt && (SysTick->VAL <= current_endpoint)) || (!lt && (SysTick->VAL >= current_endpoint))) {
            //uint32_t sta = SysTick->VAL;

            // Following line is equivalent to:
            //     ADC_StartOfConversion(ADC1); // Function call overhead is significant.
            ADC1->CR |= (uint32_t)ADC_CR_ADSTART;

            // Following line is equivalent to:
            //     while((DMA_GetFlagStatus(DMA1_FLAG_TC1)) == RESET);
            while ((DMA1->ISR & DMA1_FLAG_TC1) == RESET);

            //uint32_t stb = SysTick->VAL;
            //debugging_writec("GAP: ");
            //debugging_write_uint32(sta-stb);
            //debugging_writec("\n");

            outputs[oi]   = adc_buffer[0];
            outputs[oi+1] = adc_buffer[1];

            ++i;
            oi += 2;

            if (i < NUM_AMP_STAGES) {
                current_endpoint = st - STAGES[i];
                lt = (current_endpoint < st);
            }
            else {
                break;
            }
        }
    }

    // Close the switch again.
    //
    // Following line is equivalent to:
    //     GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 0);
    INTEGCLR_GPIO_PORT->BSRR = INTEGCLR_PIN;

    // Subsequent code is necessary to get things back to a stable state before
    // next reading (probably because it allows ADC cap to discharge?)
    //
    // Following line is equivalent to:
    //     ADC_StartOfConversion(ADC1); // Function call overhead is significant.
    ADC1->CR |= (uint32_t)ADC_CR_ADSTART;

    // Following line is equivalent to:
    //     while ((DMA_GetFlagStatus(DMA1_FLAG_TC1)) == RESET);
    while ((DMA1->ISR & DMA1_FLAG_TC1) == RESET);
}

void meter_take_averaged_raw_readings_(uint16_t *outputs, unsigned n, noise_filter_mode_t nfm, int mode)
{
    unsigned len = (mode == 0 ? NUM_AMP_STAGES*2 : 2);
    uint32_t outputs_total[len];

    uint32_t period;
    if (nfm == NOISE_FILTER_MODE_MAINS) {
        period = (800000*4)/n;
    }
    else {
        period = 0;
    }

    unsigned i;
    for (i = 0; i < len; ++i)
        outputs_total[i] = 0;

    for (i = 0; i < n; ++i) {
        uint32_t st = SysTick->VAL;
        uint32_t end = st - period;
        bool lt = (end <= st);

        if (mode == 0) {
            meter_take_raw_integrated_readings(outputs);
        }
        else {
            uint32_t vs = meter_take_raw_nonintegrated_reading();
            outputs[0] = vs & 0xFFFF;
            outputs[1] = vs >> 16;
        }
        unsigned j;
        for (j = 0; j < len; ++j) {
            outputs_total[j] += outputs[j] << 4;
        }

        while ((lt && SysTick->VAL >= end) || (!lt && SysTick->VAL <= end));
    }

    for (i = 0; i < len; ++i) {
        outputs_total[i] /= n;
        if ((outputs_total[i] & 0b1111) >= 8)
            outputs_total[i] += 8;
        outputs_total[i] >>= 4;
    }

    for (i = 0; i < len; ++i)
        outputs[i] = outputs_total[i];
}

#define ND_FILTER_120TH_STOPS      ((int)(3.5f*120.0f))
#define ND_FILTER_WHOLE_STOPS      (ND_FILTER_120TH_STOPS / 120)
#define ND_FILTER_THIRD_REMAINDER  (ND_FILTER_120TH_STOPS % (120/3))
#define ND_FILTER_EIGHTH_REMAINDER (ND_FILTER_120TH_STOPS % (120/8))
#define ND_FILTER_TENTH_REMAINDER  (ND_FILTER_120TH_STOPS % (120/10))

static ev_with_fracs_t add_extra_stops_for_nd_filter(ev_with_fracs_t evwf)
{
    uint_fast8_t ev8 = ev_with_fracs_get_ev8(evwf);
    ev8 += (8*ND_FILTER_WHOLE_STOPS) + ND_FILTER_EIGHTH_REMAINDER;
    int_fast8_t thirds = ev_with_fracs_get_thirds(evwf);
    thirds += ND_FILTER_THIRD_REMAINDER;
    if (thirds >= 3)
        thirds -= 3;
    int_fast8_t tenths = ev_with_fracs_get_tenths(evwf);
    tenths += ND_FILTER_TENTH_REMAINDER;
    if (tenths >= 10)
        tenths -= 10;
    ev_with_fracs_t ret;
    ev_with_fracs_init_from_ev8(ret, ev8);
    ev_with_fracs_set_thirds(ret, thirds);
    ev_with_fracs_set_tenths(ret, tenths);
    return ret;
}

static uint_fast8_t getv(uint16_t v)
{
    if (v % 16 >= 8)
        v += 8;
    v >>= 4;
    return (uint_fast8_t)v;
}

#define MAX12BITV 3500

//#define EXCLUDE_ND_SENSORS

#define HAS_ND_FILTER(n) \
    ((current_mode == METER_MODE_REFLECTIVE && (n) % 2 == 0) || (current_mode == METER_MODE_INCIDENT && (n) % 2 == 1))
#define LOWEST_AMPLIFICATION_WITH_ND_FILTER \
    (current_mode == METER_MODE_REFLECTIVE ? 0 : 1)
#define HIGHEST_AMPLIFICATION_WITHOUT_ND_FILTER \
    (current_mode == METER_MODE_REFLECTIVE ? NUM_AMP_STAGES-1 : NUM_AMP_STAGES-2)

ev_with_fracs_t meter_take_integrated_reading()
{
    uint16_t outputs[NUM_AMP_STAGES*2];
    meter_take_raw_integrated_readings(outputs);

//     unsigned x;
//     debugging_writec("RAW: ");
//     for (x = 0; x < NUM_AMP_STAGES*2; ++x) {
// #ifdef EXCLUDE_ND_SENSORS
//         if (HAS_ND_FILTER(x))
//             continue;
// #endif
//
//         if (x > 0) {
// #ifdef EXLCUDE_ND_SENSORS
//             debugging_writec(", ");
// #else
//             if (x % 2 == 0)
//                 debugging_writec(";  ");
//             else
//                 debugging_writec(", ");
// #endif
//         }
//         debugging_write_uint32(outputs[x]);
//     }
//     debugging_writec("\n");

    ev_with_fracs_t evs[NUM_AMP_STAGES*2];

    unsigned n, i;
    for (n = 0, i = 0; i < NUM_AMP_STAGES*2; ++i) {
#ifdef EXCLUDE_ND_SENSORS
        if (HAS_ND_FILTER(i))
            continue;
#endif

        if (! (outputs[i] < VOLTAGE_OFFSET_12BIT || outputs[i] > MAX12BITV)) {
            ev_with_fracs_t ev = get_ev100_at_voltage(getv(outputs[i]), i/2 + 1);
            if (HAS_ND_FILTER(i))
                ev = add_extra_stops_for_nd_filter(ev);
            evs[n++] = ev;
        }
    }

    if (n == 0) {
        if (outputs[HIGHEST_AMPLIFICATION_WITHOUT_ND_FILTER] < VOLTAGE_OFFSET_12BIT)
            return get_ev100_at_voltage(getv(outputs[HIGHEST_AMPLIFICATION_WITHOUT_ND_FILTER]), NUM_AMP_STAGES);
        else
            return add_extra_stops_for_nd_filter(get_ev100_at_voltage(getv(outputs[LOWEST_AMPLIFICATION_WITH_ND_FILTER]), 1));
    }
    else {
        // debugging_writec("EV10s: ");
        // unsigned j;
        // for (j = 0; j < n; ++j) {
        //     if (j != 0)
        //         debugging_writec(", ");
        //     debugging_write_uint32(ev_with_fracs_get_wholes(evs[j])*10 + ev_with_fracs_get_tenths(evs[j]));
        // }
        // debugging_writec("\n");

        return average_ev_with_fracs(evs, n);
    }
}
