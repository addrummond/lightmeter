#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_rcc.h>
#include <stm32f0xx_pwr.h>

#include <deviceconfig.h>
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
#include <meter.h>
#include <tables.h>
#include <hfsdp.h>
#include <hamming.h>

void HardFault_Handler()
{
    debugging_writec("HARD FAULT\n");
}

static __attribute__ ((unused)) void test_mic()
{
    piezo_mic_init();
    unsigned i;

    for (i = 0; i < 10; ++i)
       piezo_mic_read_buffer();

    for (i = 0;; ++i) {
        // int32_t now = SysTick->VAL;
        // piezo_mic_read_buffer();
        // goetzel_result_t r1, r2;
        // int32_t pow1, pow2, tpow;
        // goetzel2((const int16_t *)piezo_mic_buffer, PIEZO_MIC_BUFFER_N_SAMPLES, 0,
        //          HFSDP_COSCOEFF1, HFSDP_SINCOEFF1, HFSDP_COSCOEFF2, HFSDP_SINCOEFF2,
        //          &r1, &r2);
        // pow1 = goetzel_get_freq_power(&r1);
        // pow2 = goetzel_get_freq_power(&r2);
        // tpow = r1.total_power;
        // int32_t then = SysTick->VAL;
        // /*debugging_writec("G: ");
        // debugging_write_uint32(now-then);
        // debugging_writec("\n");*/
        // debugging_write_uint32(pow1);
        // debugging_writec(", ");
        // debugging_write_uint32(pow2);
        // debugging_writec("   [");
        // debugging_write_uint32(tpow);
        // debugging_writec("]\n");
        // continue;

        // piezo_mic_read_buffer();
        // unsigned j;
        // for (j = 0; j < PIEZO_MIC_BUFFER_N_SAMPLES; ++j) {
        //     debugging_write_int32(piezo_mic_buffer[j]);
        //     debugging_writec("\n");
        // }
        // debugging_writec("****\n");
        // continue;
        //
        // piezo_mic_read_buffer();
        // debugging_write_int32(piezo_get_magnitude());
        // debugging_writec("\n");
        // continue;

        //piezo_hfsdp_listen_for_masters_init();
        //continue;

        debugging_writec("Waiting...\n");
        uint8_t buf[40];
        memset8_zero(buf, sizeof(buf));
        bool decoded_successfully = piezo_read_data(buf, sizeof(buf)/sizeof(buf[0]));

        debugging_writec("DATA READ\n");

        if (! decoded_successfully) {
            debugging_writec("DECODE FAIL ");
            debugging_write_uint32(i);
            debugging_writec("\n");
            continue;
        }

        hamming_scan_for_init_sequence_result_t sr;
        sr = hamming_scan_for_init_sequence(buf, sizeof(buf));
        debugging_writec("RES: ");
        debugging_write_int32(sr.bit_index);
        debugging_writec(", ");
        debugging_write_uint32(sr.count);
        debugging_writec("\n");

        if (sr.bit_index != -1) {
            unsigned byte_index = (sr.bit_index / 8) + 1 + (sr.count*4);
            unsigned rbit_index = (8 - (sr.bit_index % 8));
            debugging_writec("SHIFTING...\n");
            hamming_bitshift_buffer_forward(buf, sizeof(buf)-1, rbit_index);
            debugging_writec("DONE SHIFTING...\n");
            unsigned j;
            debugging_writec("BYTE I: ");
            debugging_write_uint32(byte_index);
            debugging_writec("\n");
            for (j = byte_index; j < sizeof(buf); j += 4) {
                int32_t v = dehammingify_uint32(buf[j] | (buf[j+1] << 8) | (buf[j+2] << 16) | (buf[j+3] << 24));
                if (v == -1) {
                    debugging_writec("V: ERR\n");
                }
                else if (v < 0) {
                    debugging_writec("WHAT THE FUCK?!\n");
                }
                else {
                    char s[1];
                    debugging_writec("V: ");
                    s[0] = v & 0xFF;
                    debugging_write(s, 1);
                    debugging_writec(" ");
                    s[0] = (v >> 8) & 0xFF;
                    debugging_write(s, 1);
                    debugging_writec("  ");
                    s[0] = (v >> 16) & 0xFF;
                    debugging_write(s, 1);
                    debugging_writec("\n");
                }
            }
        }

        // unsigned i;
        // for (i = 0; i < (sizeof(buf)/sizeof(buf[0]))*8; ++i) {
        //     debugging_writec("B: ");
        //     debugging_write_uint32((buf[i / 8] >> (i % 8)) & 1);
        //     debugging_writec("\n");
        // }
        // debugging_writec("*****\n\n");
    }
}

static __attribute__ ((unused)) void test_speaker()
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

static __attribute__ ((unused)) void test_display()
{/*
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
*/}

static __attribute__ ((unused)) void test_accel()
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

static __attribute__ ((unused)) void show_calibration_info()
{
    meter_init();

    uint16_t outputs_refl_integ[NUM_AMP_STAGES*2];
    uint16_t outputs_inc_integ[NUM_AMP_STAGES*2];
    uint16_t outputs_refl_noninteg[2];
    uint16_t outputs_inc_noninteg[2];

    for (;;) {
        //unsigned mask = buttons_get_mask();
        //if (mask == 0)
        //    continue;
        //buttons_clear_mask();

        meter_set_mode(METER_MODE_REFLECTIVE);
        meter_take_averaged_raw_integrated_readings(outputs_refl_integ, NOISE_FILTER_MODE_MAINS, 20);
        meter_take_averaged_raw_nonintegrated_readings(outputs_refl_noninteg, NOISE_FILTER_MODE_MAINS, 20);

        meter_set_mode(METER_MODE_INCIDENT);
        meter_take_averaged_raw_integrated_readings(outputs_inc_integ, NOISE_FILTER_MODE_MAINS, 20);
        meter_take_averaged_raw_nonintegrated_readings(outputs_inc_noninteg, NOISE_FILTER_MODE_MAINS, 20);

        unsigned i;

#define OUT(arr, n) \
        for (i = 0; i < n; ++i) { \
            debugging_write_uint32(outputs_refl_integ[i]); \
            debugging_writec(", "); \
        }

        OUT(outputs_refl_integ, NUM_AMP_STAGES*2);
        OUT(outputs_inc_integ, NUM_AMP_STAGES*2);
        OUT(outputs_refl_noninteg, 2);
        OUT(outputs_inc_noninteg, 2);
#undef OUT

        debugging_writec("\n");
    }
}

static __attribute__ ((unused)) void test_meter()
{
    meter_init();
    meter_set_mode(METER_MODE_REFLECTIVE);
    uint16_t outputs[NUM_AMP_STAGES*2];
    unsigned m = 0;
    for (;;) {
        debugging_writec("----- ");
        if (m == 0)
            debugging_writec("INCIDENT -----\n");
        else
            debugging_writec("REFLECTIVE -----\n");

        meter_take_raw_integrated_readings(outputs);
        debugging_writec("V: ");
        unsigned i;
        for (i = 0; i < NUM_AMP_STAGES*2; ++i) {
            if (i != 0) {
                if (i % 2 == 0)
                    debugging_writec(";  ");
                else
                    debugging_writec(", ");
            }
            debugging_write_uint32(outputs[i]);
        }
        debugging_writec("\n");

        /*uint32_t vs = meter_take_raw_nonintegrated_reading();
        debugging_writec("NI: ");
        debugging_write_uint32(vs & 0xFFFF);
        debugging_writec(", ");
        debugging_write_uint32(vs >> 16);
        debugging_writec("\n");

        meter_set_mode(m == 0 ? METER_MODE_INCIDENT : METER_MODE_REFLECTIVE);
        m = !m;

        ev_with_fracs_t evwf = meter_take_integrated_reading();
        debugging_writec("EV 10ths: ");

        int16_t val = (((int16_t)(ev_with_fracs_get_wholes(evwf)))-5)*10 + (int16_t)ev_with_fracs_get_nearest_tenths(evwf);
        if (val < 0) {
            debugging_writec("-");
            val = -val;
        }
        debugging_write_uint32(val / 10);
        debugging_writec(".");
        debugging_write_uint32(val % 10);
        debugging_writec("\n");*/
    }
}

static __attribute__ ((unused)) void test_menu_scroll()
{
    accel_init();
    i2c_init();
    display_init();

    meter_state_t *gms = &global_meter_state;
    gms->ui_mode = UI_MODE_MAIN_MENU;
    gms->ui_mode_state.main_menu.start_line = 0;

    ui_show_interface(0);

    gms->ui_mode_state.main_menu.start_line = 0;
    int s = (int)(gms->ui_mode_state.main_menu.start_line);
    for (;;) {
        int8_t a = accel_read_register(ACCEL_REG_OUT_Y_MSB);
        if (a > -5 && a < 5)
            a = 0;

        if (a == 0)
            continue;

        if (a > 0)
            ++s;
        else if (a < 0)
            --s;

        if (s < 0)
            s += 64;
        if (s > 63)
            s -= 64;

        //gms->ui_mode_state.main_menu.start_line = (uint8_t)s;

        // Python 3: [int(round(math.exp(x/16.0)/4500000000)) for x in range(512,256,-1)][:32]
        static const int16_t delays[] = {
            17547, 16484, 15485, 14547, 13666, 12838, 12060, 11329, 10643, 9998, 9392, 8823, 8289, 7787, 7315, 6872, 6455, 6064, 5697, 5352, 5027, 4723, 4437, 4168, 3915, 3678, 3455, 3246, 3049, 2865, 2691, 2528
        };

        display_command(DISPLAY_SETSTARTLINE + s);
        unsigned j;
        int32_t pa = a;
        if (pa < 0)
            pa = -pa;
        unsigned in = pa;
        if (in >= sizeof(delays)/sizeof(delays[0]))
            in = sizeof(delays)/sizeof(delays[0]) - 1;
        int32_t delay = delays[in];
        //debugging_write_uint32(pa);
        //debugging_writec("\n");
        for (j = 0; j < delay; ++j);
    }
}

typedef enum after_release {
    AFTER_RELEASE_NOWAIT,
    AFTER_RELEASE_NOTHING,
    AFTER_RELEASE_SHOW_READING
} after_release_t;

int main()
{
    meter_state_t *gms = &global_meter_state;
    transient_meter_state_t *tms = &global_transient_meter_state;

    sysinit_init();
    initialize_global_meter_state();
    initialize_global_transient_meter_state();

    test_mic();
    for(;;);

    //accel_init();
    //meter_init();
    //meter_set_mode(METER_MODE_INCIDENT);
    //i2c_init();
    //display_init();

    uint32_t last_systick = SysTick->VAL;
    after_release_t wait_for_release = AFTER_RELEASE_NOWAIT;
    for (;;) {
        //if (sysinit_is_time_to_sleep()) {
        //    sysinit_enter_sleep_mode();
        //    sysinit_after_wakeup_init();
        //}

        uint32_t current_systick = SysTick->VAL;
        uint32_t ticks_since_ui_last_shown;
        if (current_systick > last_systick)
            ticks_since_ui_last_shown = last_systick + (SYS_TICK_MAX - current_systick);
        else
            ticks_since_ui_last_shown = last_systick - current_systick;
        last_systick = current_systick;
        ui_show_interface(ticks_since_ui_last_shown);

        if (wait_for_release != AFTER_RELEASE_NOWAIT) {
            if (buttons_get_ticks_pressed_for() == 0)
                buttons_clear_mask();
            else
                continue;

            if (wait_for_release == AFTER_RELEASE_SHOW_READING) {
                gms->ui_mode = UI_MODE_READING;
            }

            wait_for_release = AFTER_RELEASE_NOWAIT;
            continue;
        }

        unsigned mask = buttons_get_mask();
        if (mask == 2 && buttons_get_ticks_pressed_for() > 5000000) {
            // Long press of center button.

            if (gms->ui_mode == UI_MODE_MAIN_MENU) {
                // If we're currently in the menu, go back to the main screen.
                buttons_clear_mask();
                gms->ui_mode = UI_MODE_INIT;
            }
            else if (gms->ui_mode == UI_MODE_INIT) {
                // If we're on the main screen, do a reading.
                tms->last_ev_with_fracs = meter_take_integrated_reading();

                debugging_writec("EV10: ");
                debugging_write_uint32(ev_with_fracs_get_wholes(tms->last_ev_with_fracs)*10 + ev_with_fracs_get_nearest_tenths(tms->last_ev_with_fracs));
                debugging_writec("\n");

                ev_with_fracs_init_from_wholes(tms->shutter_speed, 10);
                ev_with_fracs_t isoev;
                ev_with_fracs_init_from_thirds(isoev, gms->iso);
                tms->aperture = shutter_speed_given_aperture_iso_ev(tms->shutter_speed, isoev, tms->last_ev_with_fracs);
                tms->exposure_ready = true;
                gms->ui_mode = UI_MODE_METERING;
                wait_for_release = AFTER_RELEASE_SHOW_READING;
            }
        }
        else if (mask == 2 && buttons_get_ticks_pressed_for() == 0) {
            buttons_clear_mask();
            gms->ui_mode = UI_MODE_MAIN_MENU;
            gms->ui_mode_state.main_menu.start_line = 0;
        }
    }
}
