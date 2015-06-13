#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_adc.h>
#include <stm32f0xx_misc.h>

#include <tables.h>
#include <deviceconfig.h>
#include <meter.h>
#include <debugging.h>

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

    // TODO: config to use both channels.
    ADC_ChannelConfig(ADC1, ADC_Channel_1, ADC_SampleTime_1_5Cycles);
    ADC_GetCalibrationFactor(ADC1);

    ADC_Cmd(ADC1, ENABLE);
    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));
}

static meter_mode_t current_mode;

#define MODE_TO_DIODESW(m) ((m) == METER_MODE_INCIDENT)
#define MODE_OTHER(m)      ((m) == METER_MODE_INCIDENT ? METER_MODE_REFLECTIVE : METER_MODE_INCIDENT)

void meter_set_mode(meter_mode_t mode)
{
    GPIO_WriteBit(DIODESW_GPIO_PORT, DIODESW_PIN, MODE_TO_DIODESW(mode));
    current_mode = mode;
}

uint32_t meter_take_raw_nonintegrated_reading()
{
    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 1);
    // Wait a bit for things to stablize.
    unsigned i;
    for (i = 0; i < 3000; ++i);

    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));
    ADC_StartOfConversion(ADC1);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}

#define st(x) STAGE ## x ##_TICKS,
static uint32_t STAGES[] = {
    FOR_EACH_AMP_STAGE(st)
};
#undef st

void meter_take_raw_integrated_readings(uint16_t *outputs)
{
    uint32_t ends[NUM_AMP_STAGES];

    // Close switch to discharge integrating cap.
    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 1);
    unsigned i;
    for (i = 0; i < 3000; ++i); // Leave some time for cap to discharge.

    //ADC1->CR |= (uint32_t)ADC_CR_ADSTART;
    //while ((ADC1->ISR & ADC_FLAG_EOC) == RESET);
    //uint16_t baseline = ADC1->DR;
    //debugging_writec("B: ");
    //debugging_write_uint32(baseline);
    //debugging_writec("\n");

    // Open the switch.

    // Following line is equivalent to:
    //    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 0);
    INTEGCLR_GPIO_PORT->BRR = INTEGCLR_PIN;

    // From here to start of first ADC conversion currently takes 66 cycles.
    // 107 cycles to end of ADC conversion.
    uint32_t st = SysTick->VAL;

    //uint32_t st2 = SysTick->VAL;
    //debugging_writec("GAP: ");
    //debugging_write_uint32(st-st2);
    //debugging_writec("\n");

    // Determine value of SysTick for each endpoint.
    for (i = 0; i < NUM_AMP_STAGES; ++i)
        ends[i] = st - STAGES[i];

    // Appears not to be necessary. (Has no discernable effect on accuracy.)
    //
    // while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));

    //uint32_t st2 = SysTick->VAL;
    //debugging_writec("GAP: ");
    //debugging_write_uint32(st-st2);
    //debugging_writec("\n");

    // Read cap voltage at each stage.
    for (i = 0; i < NUM_AMP_STAGES;) {
        if (SysTick->VAL <= ends[i]) {
            //uint32_t stb = SysTick->VAL;
            //debugging_writec("GAP: ");
            //debugging_write_uint32(st-stb);
            //debugging_writec("\n");

            // Following line is equivalent to:
            //     ADC_StartOfConversion(ADC1); // Function call overhead is significant.
            ADC1->CR |= (uint32_t)ADC_CR_ADSTART;

            // Following line is equivalent to:
            //     while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET); // Function call overhead is signficant.
            while ((ADC1->ISR & ADC_FLAG_EOC) == RESET);

            // Following line is equivalent to:
            //     outputs[i] = ADC_GetConversionValue(ADC1);
            outputs[i] = ADC1->DR;

            //uint32_t stb = SysTick->VAL;
            //debugging_writec("GAP: ");
            //debugging_write_uint32(st-stb);
            //debugging_writec("\n");

            ++i;
        }
    }

    // Close the switch again.
    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 1);
}

void meter_take_averaged_raw_integrated_readings(uint16_t *outputs, unsigned n)
{
    uint32_t outputs_total[NUM_AMP_STAGES];

    unsigned i;
    for (i = 0; i < NUM_AMP_STAGES; ++i)
        outputs_total[i] = 0;
    for (i = 0; i < n; ++i) {
        meter_take_raw_integrated_readings(outputs);
        unsigned j;
        for (j = 0; j < NUM_AMP_STAGES; ++j) {
            outputs_total[j] += outputs[j] << 4;
        }
    }
    for (i = 0; i < NUM_AMP_STAGES; ++i) {
        outputs_total[i] /= n;
        if ((outputs_total[i] & 0b1111) >= 8)
            outputs_total[i] += 8;
        outputs_total[i] >>= 4;
    }

    for (i = 0; i < NUM_AMP_STAGES; ++i)
        outputs[i] = outputs_total[i];
}

static uint_fast8_t getv(uint16_t v)
{
    if (v % 16 >= 8)
        v += 16;
    v >>= 4;
    return (uint_fast8_t)v;
}

#define MAX12BITV 3500
ev_with_fracs_t meter_take_integrated_reading()
{
    uint16_t outputs[NUM_AMP_STAGES];
    meter_take_raw_integrated_readings(outputs);

    // unsigned x;
    // debugging_writec("RAW: ");
    // for (x = 0; x < NUM_AMP_STAGES; ++x) {
    //     if (x > 0)
    //         debugging_writec(", ");
    //     debugging_write_uint32(outputs[x]);
    // }
    // debugging_writec("\n");

    ev_with_fracs_t evs[NUM_AMP_STAGES];

    unsigned n = 0, i;
    for (i = 0; i < NUM_AMP_STAGES; ++i) {
        if (! (outputs[i] < VOLTAGE_OFFSET_12BIT || outputs[i] > MAX12BITV)) {
            evs[n++] = get_ev100_at_voltage(getv(outputs[i]), i + 1);
        }
    }
    if (n == 0) {
        if (outputs[NUM_AMP_STAGES-1] < VOLTAGE_OFFSET_12BIT)
            return get_ev100_at_voltage(getv(outputs[NUM_AMP_STAGES-1]), NUM_AMP_STAGES);
        else
            return get_ev100_at_voltage(getv(outputs[0]), 1);
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
