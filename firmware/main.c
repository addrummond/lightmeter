#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_pwr.h>

#include <i2c.h>
#include <piezo.h>
#include <display.h>
#include <accel.h>
#include <meter.h>
#include <ui.h>
#include <state.h>
#include <debugging.h>
#include <piezo.h>
#include <buttons.h>
#include <bitmaps/bitmaps.h>
#include <mymemset.h>
#include <sysinit.h>

static void test_mic()
{
    piezo_mic_init();
    piezo_hfsdp_listen_for_masters_init();
    debugging_writec("Init heard!\n");
    for (;;);
}

static void test_speaker()
{
    piezo_out_init();
    piezo_set_period(1, (SystemCoreClock / 1000) - 1);
    piezo_set_period(2, (SystemCoreClock / 1500) - 1);
    piezo_turn_on(3);
    for (;;) {
        piezo_pause(1);
        unsigned i;
        for (i = 0; i < 2000000; ++i);
        piezo_unpause(1);
        for (i = 0; i < 2000000; ++i);
        debugging_writec("loop\n");
    }

    debugging_writec("Piezo init complete\n");
}

static void test_display()
{
    display_init();
    display_clear();

#define gms global_meter_state
#define tms global_transient_meter_state
    gms.bcd_iso_digits[0] = 1;
    gms.bcd_iso_digits[1] = 0;
    gms.bcd_iso_digits[2] = 0;
    gms.bcd_iso_length = 3;
    ev_with_fracs_set_ev8(gms.stops_iso, 8*3);
    gms.priority = SHUTTER_PRIORITY;
    gms.exp_comp = 0;
    gms.ui_mode = UI_MODE_MAIN_MENU;
    gms.meter_mode = METER_MODE_INCIDENT;
    gms.precision_mode = PRECISION_MODE_TENTH;
    ev_with_fracs_set_ev8(gms.priority_aperture, 8*8);
    ev_with_fracs_set_ev8(gms.priority_shutter_speed, 8*8);

    ev_with_fracs_set_ev8(tms.last_ev_with_fracs, 8*8);
    ev_with_fracs_set_ev8(tms.aperture, 8*8);
    ev_with_fracs_set_ev8(tms.shutter_speed, 8*8);
    tms.exposure_ready = true;
    tms.op_amp_resistor_stage = 2;
    gms.ui_mode_state.main_menu.voffset = 3;
#undef gms
#undef tms
    ui_show_interface();
}

static void test_accel()
{
    display_init();
    display_clear();
    accel_init();

    uint8_t pa[6];
    uint8_t zpa[6];
    memset8_zero(pa, sizeof(pa));
    memset8_zero(zpa, sizeof(zpa));
    display_bwrite_8px_char(CHAR_8PX_F, pa, 1, 0);
    //display_write_page_array(pa, 6, 1, 20/*v/4*/, 3);

    uint8_t lastx = 0;
    uint32_t start, end;
    for (;;) {
        start = SysTick->VAL;
        end = SysTick->VAL - 400000;
        int8_t v = accel_read_register(ACCEL_REG_OUT_X_MSB);
        //debugging_writec("V: ");
        //debugging_write_uint32(v);
        //debugging_writec("\n");

        int lastx = 64-v;
        if (lastx < 0)
            lastx = 0;
        if (lastx > 127)
            lastx = 127;

        display_write_page_array(pa, 6, 1, lastx, 3);

        if (end > start)
            while (SysTick->VAL < start);
        while (SysTick->VAL > end);

        display_write_page_array(zpa, 6, 1, lastx, 3);
    }
}

static void test_meter()
{
    meter_init();
    meter_set_mode(METER_MODE_INCIDENT);
    uint16_t outputs[METER_NUMBER_OF_INTEGRATING_STAGES];
    for (;;) {
        meter_take_averaged_raw_integrated_readings(outputs, 10);
        debugging_writec("V: ");
        unsigned i;
        for (i = 0; i < METER_NUMBER_OF_INTEGRATING_STAGES; ++i) {
            if (i != 0)
                debugging_writec(", ");
            debugging_write_uint32(outputs[i]);
        }
        debugging_writec("\n");
    }
}

int main()
{
    sysinit_init();

    //test_mic();
    //test_display();
    //test_accel();

    for (;;);
}
