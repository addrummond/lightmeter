void sensor_init_config()
{
    // Configure amp stage selection pins.
    GPIO_InitTypeDef gpi;
    gpi.GPIO_Pin = STG1_PIN;
    gpi.GPIO_Mode = GPIO_Mode_OUT;
    gpi.GPIO_Speed = GPIO_Speed_Level_1;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(STG1_GPIO_PORT, &gpi);
    gpi.GPIO_Pin = STG2_PIN;
    GPIO_Init(STG2_GPIO_PORT, &gpi);
}

void sensor_init_adc()
{
    GPIO_InitTypeDef gpi;
    ADC_InitTypeDef adci;

    gpi.GPIO_Pin = MEASURE1_GPIO_PIN;
    gpi.GPIO_Mode = GPIO_Mode_AN;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(MEASURE1_GPIO_PORT, &gpi);
    gpi.GPIO_Pin = MEASURE2_GPIO_PIN;
    GPIO_Init(MEASURE2_GPIO_PORT, &gpi);

    adci.ADC_Resolution = ADC_Resolution_12b;
    adci.ADC_ContinuousConvMode = DISABLE;
    adci.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    adci.ADC_DataAlign = ADC_DataAlign_Right;
    adci.ADC_ScanDirection = ADC_ScanDirection_Upward;
    ADC_Init(ADC1, &adci);
    ADC_Init(ADC2, &adci);

    ADC_ChannelConfig(ADC1, MEASURE1_ADC_CHANNEL, ADC_SampleTime_239_5Cycles);
    ADC_ChannelConfig(ADC2, MEASURE2_ADC_CHANNEL, ADC_SampleTime_239_5Cycles);

    ADC_GetCalibrationFactor(ADC1);
    ADC_GetCalibrationFactor(ADC2);

    ADC_Cmd(ADC1, ENABLE);
    ADC_Cmd(ADC2, ENABLE);
}

// 0 is power down mode.
void sensor_select_amp_stage(unsigned stage)
{
    GPIO_Write_Bit(STG1_GPIO_PORT, STG1_PIN, stage & 1);
    GPIO_Write_Bit(STG2_GPIO_PORT, STG2_PIN, (stage >> 1) & 1);
}

void sensor_reading()
{
    for (unsigned i = 1; i < 4, ++i) {
        sensor_select_amp_stage(i);


    }
}
