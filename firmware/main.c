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

// Mic: 1.3K
// Vcc to mic: 10K
// Midpoint:

#define N_SAMPLES 256

static int8_t samples[N_SAMPLES*2];
int main()
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);

    debugging_writec("Hello World!\n");

    piezo_mic_init();
    for (;;) {
        unsigned i;

        int16_t *samples16 = (int16_t *)samples;
        for (i = 0; i < N_SAMPLES; ++i, piezo_mic_wait_on_ready()) {
            samples16[i] = (int16_t)(piezo_mic_get_reading()) - (4096/2);
        }
        samples16[0] = samples16[1];

        // 12 bits to 8.
        int16_t raw_max = -(4096/2);
        for (i = 0; i < N_SAMPLES; ++i) {
            int16_t v = samples16[i];
            if (v < 0)
                v = -v;
            if (v > raw_max)
                raw_max = v;
        }
        int16_t ratio = 127/raw_max;
        debugging_writec("Rat: ");
        debugging_write_uint16(ratio);
        debugging_writec("\n");
        if (ratio > 1) {
            for (i = 0; i < N_SAMPLES; ++i)
                samples16[i] *= ratio;
        }
        raw_max = -(4096/2);
        for (i = 0; i < N_SAMPLES; ++i) {
            int16_t v = samples16[i];
            if (v < 0)
                v = -v;
            if (v > raw_max)
                raw_max = v;
        }
        unsigned shift = 0;
        if (raw_max >= 1024) {
            shift = 4;
        }
        else if (raw_max >= 512) {
            shift = 3;
        }
        else if (raw_max >= 256) {
            shift = 2;
        }
        else if (raw_max >= 128) {
            shift = 1;
        }
        for (i = 0; i < N_SAMPLES; ++i) {
            samples[i] = samples16[i] >> shift;
        }

        debugging_writec("--start--\n");
        for (i = 0; i < N_SAMPLES; ++i) {
            debugging_write_uint8(samples[i]);
            debugging_writec("\n");
        }
        debugging_writec("--end--\n");
        continue;

        int8_t *imaginary = samples + N_SAMPLES;
        for (i = 0; i < N_SAMPLES; ++i) {
            imaginary[i] = 0;
        }

        uint32_t sq = 0;
        for (i = 0; i < N_SAMPLES; ++i)
            sq += samples[i] * samples[i];

        uint32_t mag = isqrt(sq);

        // Find first formant.
        fix_fft(samples, imaginary, 9, 0);
        int32_t max = 0;
        unsigned maxi = 0;
        for (i = 0; i < N_SAMPLES/2; ++i) {
            int32_t e = samples[i]*samples[i] + imaginary[i]*imaginary[i];

            if (max < e) {
                max = e;
                maxi = i;
            }
        }

        debugging_writec("Mag: ");
        debugging_write_uint16(mag);
        debugging_writec(" ~ ");
        debugging_write_uint16(maxi);
        debugging_writec("\n");
        //unsigned i;
        //for (i = 0; i < 200000; ++i);
    }

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
