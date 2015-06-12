#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_pwr.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_misc.h>
#include <stm32f0xx_tim.h>
#include <sysinit.h>
#include <stdbool.h>
#include <debugging.h>
#include <display.h>
#include <deviceconfig.h>

#include <i2c.h>
#include <buttons.h>

static bool time_to_sleep = false;

void TIM3_IRQHandler()
{
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    time_to_sleep = true;
}

bool sysinit_is_time_to_sleep()
{
    return time_to_sleep;
}

void sysinit_reset_sleep_counter()
{
    TIM3->CNT = 0;
}

static void initialize_sleep_timer()
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = TIM3_IRQn;
    nvic.NVIC_IRQChannelPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    const uint16_t PRE = 6000;
    TIM_TimeBaseInitTypeDef tbi;
    tbi.TIM_Period = 10000;
    tbi.TIM_Prescaler = 0;
    tbi.TIM_ClockDivision = 0;
    tbi.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &tbi);

    TIM_PrescalerConfig(TIM3, PRE, TIM_PSCReloadMode_Immediate);

    TIM_ClearITPendingBit(TIM3, TIM_IT_Update); // Stop interrupt being triggered immediately.
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

void sysinit_init()
{
    initialize_sleep_timer();

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);

    // Make sure photosensor current flows through resistor by default.
    GPIO_InitTypeDef gpi;
    gpi.GPIO_Pin = INTEGCLR_PIN;
    gpi.GPIO_Mode = GPIO_Mode_OUT;
    gpi.GPIO_Speed = GPIO_Speed_Level_3;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(INTEGCLR_GPIO_PORT, &gpi);
    GPIO_WriteBit(INTEGCLR_GPIO_PORT, INTEGCLR_PIN, 1);

    // Only required for Seeed prototype 2 -- enable LREG so that the uc isn't
    // fighting the connection between the LREG's enable pin and VCC.
    gpi.GPIO_Pin = GPIO_Pin_5;
    gpi.GPIO_Mode = GPIO_Mode_IN;
    gpi.GPIO_Speed = GPIO_Speed_Level_1;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &gpi);

    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
    SysTick_Config(SYS_TICK_MAX+1);
    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = SysTick_IRQn;
    nvic.NVIC_IRQChannelPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    i2c_init();
    buttons_setup();
}

static void set_pins_low_analogue()
{
    GPIO_InitTypeDef gpi;
#define ALLA ( GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 \
               | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 \
               | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 \
               | GPIO_Pin_15 )
#define ALLB ( GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 \
               | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 )
    GPIO_StructInit(&gpi);
    gpi.GPIO_Pin = ALLA;
    if (GPIOA == USB_DPLUS_GPIO_PORT)
        gpi.GPIO_Pin &= ~USB_DPLUS_PIN;
    if (GPIOA == USB_DMINUS_GPIO_PORT)
        gpi.GPIO_Pin &= ~USB_DMINUS_PIN;
    if (GPIOA == PUSHBUTTON1_GPIO_PORT)
        gpi.GPIO_Pin &= ~PUSHBUTTON1_PIN;
    if (GPIOA == PUSHBUTTON2_GPIO_PORT)
        gpi.GPIO_Pin &= ~PUSHBUTTON2_PIN;
    GPIO_Init(GPIOA, &gpi);
    gpi.GPIO_Pin = ALLB;
    if (GPIOB == USB_DPLUS_GPIO_PORT)
        gpi.GPIO_Pin &= ~USB_DPLUS_PIN;
    if (GPIOB == USB_DMINUS_GPIO_PORT)
        gpi.GPIO_Pin &= ~USB_DMINUS_PIN;
    if (GPIOB == PUSHBUTTON1_GPIO_PORT)
        gpi.GPIO_Pin &= ~PUSHBUTTON1_PIN;
    if (GPIOB == PUSHBUTTON2_GPIO_PORT)
        gpi.GPIO_Pin &= ~PUSHBUTTON2_PIN;
    GPIO_Init(GPIOB, &gpi);
#undef ALLA
#undef ALLB
}

void sysinit_enter_sleep_mode()
{
    TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
    sysinit_reset_sleep_counter();
    time_to_sleep = false;

    display_ramp_down_contrast(192000);
    display_command(DISPLAY_DISPLAYOFF);
    set_pins_low_analogue();
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_SLEEPEntry_WFI);
}

void sysinit_after_wakeup_init()
{
    sysinit_init();
}
