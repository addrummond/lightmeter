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
    // Init DIODESW, INTEGCLR and GB1/2 GPIO pins.
    //
    gpi.GPIO_Pin = DIODESW_PIN;
    gpi.GPIO_Mode = GPIO_Mode_OUT;
    gpi.GPIO_Speed = GPIO_Speed_Level_1;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(DIODESW_GPIO_PORT, &gpi);
    gpi.GPIO_Pin = INTEGCLR_PIN;
    GPIO_Init(INTEGCLR_GPIO_PORT, &gpi);
    gpi.GPIO_Pin = GB1_PIN;
    GPIO_Init(GB1_GPIO_PORT, &gpi);
    gpi.GPIO_Pin = GB2_PIN;
    GPIO_Init(GB2_GPIO_PORT, &gpi);

    //
    // Set GB1/2 low, because with the Phillips IC the relevant pins should
    // really be connected to ground.
    //
    GPIO_WriteBit(GB1_GPIO_PORT, GB1_PIN, 0);
    GPIO_WriteBit(GB2_GPIO_PORT, GB2_PIN, 0);

    //
    // Init ADC.
    //
    ADC_InitTypeDef adci;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    gpi.GPIO_Pin = GPIO_Pin_0;
    gpi.GPIO_Mode = GPIO_Mode_AN;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpi);
    gpi.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOA, &gpi);

    ADC_DeInit(ADC1);

    ADC_StructInit(&adci);
    adci.ADC_Resolution = ADC_Resolution_12b;
    adci.ADC_ContinuousConvMode = DISABLE;
    adci.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    adci.ADC_DataAlign = ADC_DataAlign_Right;
    adci.ADC_ScanDirection = ADC_ScanDirection_Upward;
    ADC_Init(ADC1, &adci);

    // TODO: config to use both channels.
    ADC_ChannelConfig(ADC1, ADC_Channel_0, ADC_SampleTime_13_5Cycles);
    ADC_GetCalibrationFactor(ADC1);

    ADC_Cmd(ADC1, ENABLE);
}

void meter_set_mode(meter_mode_t mode)
{
    GPIO_WriteBit(DIODESW_GPIO_PORT, DIODESW_PIN, mode == METER_MODE_REFLECTIVE);
}

uint32_t meter_take_raw_nonintegrated_reading()
{
    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 0);

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

    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));

    // Close switch, discharge integrating cap.
    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 0);
    unsigned i;
    for (i = 0; i < 1000; ++i); // Leave some time for cap to discharge.

    // Open the switch.
    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 1);

    // Determine value of SysTick for each endpoint.
    uint32_t st = SysTick->VAL;
    for (i = 0; i < NUM_AMP_STAGES; ++i)
        ends[i] = st - STAGES[i];

    // Read cap voltage at each stage.
    for (i = 0; i < NUM_AMP_STAGES;) {
        if (SysTick->VAL <= ends[i]) {
            ADC_StartOfConversion(ADC1);
            while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
            outputs[i] = ADC_GetConversionValue(ADC1);

            ++i;
        }
    }

    // Close the switch again.
    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 0);
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

ev_with_fracs_t meter_take_integrated_reading()
{
    uint16_t outputs[NUM_AMP_STAGES];
    meter_take_raw_integrated_readings(outputs);

    debugging_writec("RAW: ");
    unsigned i;
    for (i = 0; i < NUM_AMP_STAGES; ++i) {
        debugging_write_uint32(outputs[i]);
        debugging_writec(", ");
    }
    debugging_writec("\n");

    unsigned stage = NUM_AMP_STAGES-1;
    while (outputs[stage] > 3500 && stage > 0)
        --stage;
    uint16_t v = outputs[stage];
    if (v % 16 >= 8)
        v += 16;
    v >>= 4;
    ++stage;

    debugging_writec("STG: ");
    debugging_write_uint32(stage);
    debugging_writec("\n");

    return get_ev100_at_voltage((uint_fast8_t)v, stage);
}
