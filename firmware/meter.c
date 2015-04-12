#include <stm32f0xx_gpio.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_adc.h>
#include <stm32f0xx_misc.h>

#include <deviceconfig.h>
#include <meter.h>

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

static const uint16_t STAGES[] = {
    480/400, 16320/400, 32160/400, 48000/400
};
// Check that number of stages defined in header file is correct.
static const uint8_t STAGES_test_dummy[(METER_NUMBER_OF_INTEGRATING_STAGES == sizeof(STAGES)/sizeof(STAGES[0]))-1];

void meter_take_raw_integrated_readings(uint16_t *outputs)
{
    uint32_t ends[sizeof(STAGES)/sizeof(STAGES[0])];

    while (! ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));

    // Close switch, discharge integrating cap.
    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 0);
    unsigned i;
    for (i = 0; i < 1000; ++i); // Leave some time for cap to discharge.

    // Open the switch.
    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 1);

    // Determine value of SysTick for each endpoint.
    uint32_t st = SysTick->VAL;
    for (i = 0; i < sizeof(STAGES)/sizeof(STAGES[0]); ++i)
        ends[i] = st - STAGES[i];

    // Read cap voltage at each stage.
    for (i = 0; i < sizeof(STAGES)/sizeof(STAGES[0]);) {
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
    uint32_t outputs_total[METER_NUMBER_OF_INTEGRATING_STAGES];

    unsigned i;
    for (i = 0; i < METER_NUMBER_OF_INTEGRATING_STAGES; ++i)
        outputs_total[i] = 0;
    for (i = 0; i < n; ++i) {
        meter_take_raw_integrated_readings(outputs);
        unsigned j;
        for (j = 0; j < METER_NUMBER_OF_INTEGRATING_STAGES; ++j) {
            outputs_total[j] += outputs[j] << 4;
        }
    }
    for (i = 0; i < METER_NUMBER_OF_INTEGRATING_STAGES; ++i) {
        outputs_total[i] /= n;
        if ((outputs_total[i] & 0b1111) >= 8)
            outputs_total[i] += 8;
        outputs_total[i] >>= 4;
    }

    for (i = 0; i < METER_NUMBER_OF_INTEGRATING_STAGES; ++i)
        outputs[i] = outputs_total[i];
}

void meter_deinit()
{

}
