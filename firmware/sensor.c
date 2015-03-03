static void sensor_init_config()
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

// 0 is power down mode.
static void sensor_select_amp_stage(unsigned stage)
{
    GPIO_Write_Bit(STG1_GPIO_PORT, STG1_PIN, stage & 1);
    GPIO_Write_Bit(STG1_GPIO_PORT, STG1_PIN, (stage >> 1) & 1);
}
