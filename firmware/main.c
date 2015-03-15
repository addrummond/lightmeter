#include <stm32f0xx.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_rcc.h>

#include <i2c.h>
#include <piezo.h>
#include <display.h>
#include <ui.h>
#include <state.h>
#include <debugging.h>

#include <fix_fft.h>

// See http://www.finesse.demon.co.uk/steven/sqrt.html
static uint32_t isqrt(uint32_t n)
{
   uint32_t root = 0, bit, trial;
   bit = (n >= 0x10000) ? 1<<30 : 1<<14;
   do
   {
      trial = root+bit;
      if (n >= trial)
      {
         n -= trial;
         root = trial+bit;
      }
      root >>= 1;
      bit >>= 2;
   } while (bit);
   return root;
}

//static int8_t samples[PIEZO_MIC_BUFFER_SIZE_BYTES];
int main()
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);

    debugging_writec("Hello World!\n");

    piezo_mic_init();
    piezo_hfsdp_listen_for_masters_init();
    debugging_writec("Init heard!\n");


    /*for (;;) {
        piezo_mic_read_buffer(samples);
        uint32_t mag = isqrt(piezo_mic_buffer_get_sqmag(samples));
        piezo_mic_buffer_fft(samples);
        unsigned first_formant = 0, second_formant = 0;
        piezo_fft_buffer_get_12formants(samples, &first_formant, &second_formant);

        debugging_writec("Mag: ");
        debugging_write_uint16(mag);
        debugging_writec(" ~ ");
        debugging_write_uint16(first_formant);
        debugging_writec(", ");
        debugging_write_uint16(second_formant);
        debugging_writec("\n");

        //unsigned i;
        //for (i = 0; i < 200000; ++i);
    }*/

    /*piezo_out_init();
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
    }*/

    debugging_writec("Piezo init complete\n");

    i2c_init();
    display_init();

    //
    // Temporary test code.
    //
    // Sets bogus UI state to show some text.
    //

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

    for (;;);
}
